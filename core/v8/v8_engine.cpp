/*
 * v8_engine.cpp — Real V8/libnode engine implementation.
 *
 * Wraps V8 Isolate creation, script execution, and module binding.
 * Links against libnode.dylib (Node.js as a shared library).
 */
#include "v8_engine.h"

#include <v8.h>
#include <libplatform/libplatform.h>
#include <cstdlib>
#include <cstring>
#include <string>

/* ── V8 platform (global, init once) ─────────────── */

static v8::Platform* g_platform = nullptr;
static bool          g_initialized = false;

static void ensure_v8_init() {
    if (g_initialized) return;
    g_platform = v8::platform::NewDefaultPlatform().release();
    v8::V8::InitializePlatform(g_platform);
    v8::V8::Initialize();
    g_initialized = true;
}

/* ── Isolate wrapper ─────────────────────────────── */

struct V8Isolate {
    v8::Isolate*              isolate;
    v8::Persistent<v8::Context> context;
    v8::Isolate::CreateParams  params;
    size_t                     heap_mb;
    size_t                     stack_kb;
    /* TODO: libnode environment for Node.js API support */
};

/* ── Snapshot loading ─────────────────────────────── */

V8Isolate* v8_isolate_create_with_snapshot(size_t heap_mb, size_t stack_kb,
                                            const char* snapshot_path) {
    ensure_v8_init();

    V8Isolate* eng = (V8Isolate*)calloc(1, sizeof(V8Isolate));
    eng->heap_mb  = heap_mb;
    eng->stack_kb = stack_kb;

    v8::Isolate::CreateParams params;
    params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    params.constraints.set_max_old_generation_size_in_bytes(
        heap_mb * 1024 * 1024);

    /* Load snapshot blob from file */
    FILE* f = fopen(snapshot_path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* blob = (char*)malloc(sz);
        fread(blob, 1, sz, f);
        fclose(f);
        params.snapshot_blob = new v8::StartupData{blob, (int)sz};
    }

    eng->params = params;
    eng->isolate = v8::Isolate::New(params);

    v8::Isolate::Scope isolate_scope(eng->isolate);
    v8::HandleScope    handle_scope(eng->isolate);

    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(eng->isolate);
    global->Set(eng->isolate, "nexa", v8::ObjectTemplate::New(eng->isolate));
    v8::Local<v8::Context> ctx = v8::Context::New(eng->isolate, nullptr, global);
    eng->context.Reset(eng->isolate, ctx);

    return eng;
}

V8Isolate* v8_isolate_create(size_t heap_mb, size_t stack_kb) {
    return v8_isolate_create_with_snapshot(heap_mb, stack_kb, nullptr);
}

/* ── Destroy ──────────────────────────────────────── */

void v8_isolate_destroy(V8Isolate* eng) {
    if (eng) free(eng);
}

/* ── Script execution ────────────────────────────── */

static char* to_cstring(v8::Isolate* iso, v8::Local<v8::Value> val) {
    if (val.IsEmpty() || val->IsUndefined() || val->IsNull()) {
        return strdup("null");
    }
    v8::String::Utf8Value utf8(iso, val);
    return strdup(*utf8 ? *utf8 : "");
}

char* v8_isolate_run(V8Isolate* eng, const char* script) {
    v8::Isolate* iso = eng->isolate;
    v8::Isolate::Scope isolate_scope(iso);
    v8::HandleScope    handle_scope(iso);

    v8::Local<v8::Context> ctx = eng->context.Get(iso);
    v8::Context::Scope context_scope(ctx);

    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(iso, script).ToLocalChecked();

    v8::MaybeLocal<v8::Script> compiled = v8::Script::Compile(ctx, source);
    if (compiled.IsEmpty()) {
        return strdup("{\"error\": \"compilation failed\"}");
    }

    v8::MaybeLocal<v8::Value> result = compiled.ToLocalChecked()->Run(ctx);
    if (result.IsEmpty()) {
        return strdup("{\"error\": \"execution failed\"}");
    }

    return to_cstring(iso, result.ToLocalChecked());
}

char* v8_isolate_call(V8Isolate* eng, const char* func_name,
                       const char* json_args) {
    v8::Isolate* iso = eng->isolate;
    v8::Isolate::Scope isolate_scope(iso);
    v8::HandleScope    handle_scope(iso);

    v8::Local<v8::Context> ctx = eng->context.Get(iso);
    v8::Context::Scope context_scope(ctx);

    /* Get the global function */
    v8::Local<v8::String> fn_name =
        v8::String::NewFromUtf8(iso, func_name).ToLocalChecked();
    v8::MaybeLocal<v8::Value> maybe_fn = ctx->Global()->Get(ctx, fn_name);

    if (maybe_fn.IsEmpty() || !maybe_fn.ToLocalChecked()->IsFunction()) {
        return strdup("{\"error\": \"function not found\"}");
    }

    v8::Local<v8::Function> fn = maybe_fn.ToLocalChecked().As<v8::Function>();

    /* Prepare arguments */
    v8::Local<v8::Value> argv[1];
    argv[0] = v8::String::NewFromUtf8(iso, json_args).ToLocalChecked();

    v8::MaybeLocal<v8::Value> result = fn->Call(ctx, ctx->Global(), 1, argv);
    if (result.IsEmpty()) {
        return strdup("{\"error\": \"call failed\"}");
    }

    return to_cstring(iso, result.ToLocalChecked());
}

char* v8_isolate_load(V8Isolate* eng, const char* name,
                       const char* script) {
    /* Load = compile the script, exposing its global functions for later call() */
    v8::Isolate* iso = eng->isolate;
    v8::Isolate::Scope isolate_scope(iso);
    v8::HandleScope    handle_scope(iso);

    v8::Local<v8::Context> ctx = eng->context.Get(iso);
    v8::Context::Scope context_scope(ctx);

    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(iso, script).ToLocalChecked();
    v8::MaybeLocal<v8::Script> compiled = v8::Script::Compile(ctx, source);

    if (compiled.IsEmpty()) {
        return strdup("{\"error\": \"load compilation failed\"}");
    }

    compiled.ToLocalChecked()->Run(ctx);
    return strdup("\"loaded\"");
}

/* ── Module binding ──────────────────────────────── */

void v8_isolate_bind(V8Isolate* eng, const char* name, void* fn_table) {
    v8::Isolate* iso = eng->isolate;
    v8::Isolate::Scope isolate_scope(iso);
    v8::HandleScope    handle_scope(iso);

    v8::Local<v8::Context> ctx = eng->context.Get(iso);
    v8::Context::Scope context_scope(ctx);

    /* Get nexa namespace */
    v8::Local<v8::Object> nexa_obj;
    v8::MaybeLocal<v8::Value> maybe_nexa = ctx->Global()->Get(ctx,
        v8::String::NewFromUtf8(iso, "nexa").ToLocalChecked());
    if (maybe_nexa.IsEmpty() || !maybe_nexa.ToLocalChecked()->IsObject()) {
        nexa_obj = v8::Object::New(iso);
        ctx->Global()->Set(ctx,
            v8::String::NewFromUtf8(iso, "nexa").ToLocalChecked(), nexa_obj);
    } else {
        nexa_obj = maybe_nexa.ToLocalChecked().As<v8::Object>();
    }

    /* Create placeholder object for the module */
    v8::Local<v8::Object> module = v8::Object::New(iso);
    module->Set(ctx,
        v8::String::NewFromUtf8(iso, "_native").ToLocalChecked(),
        v8::External::New(iso, fn_table));
    nexa_obj->Set(ctx,
        v8::String::NewFromUtf8(iso, name).ToLocalChecked(), module);
}

void v8_isolate_set_global(V8Isolate* eng, const char* name,
                            const char* json) {
    v8::Isolate* iso = eng->isolate;
    v8::Isolate::Scope isolate_scope(iso);
    v8::HandleScope    handle_scope(iso);

    v8::Local<v8::Context> ctx = eng->context.Get(iso);
    v8::Context::Scope context_scope(ctx);

    ctx->Global()->Set(ctx,
        v8::String::NewFromUtf8(iso, name).ToLocalChecked(),
        v8::String::NewFromUtf8(iso, json).ToLocalChecked());
}

/* ── DirectBuffer (placeholder — requires shared memory) ─ */

void* v8_isolate_direct_buffer(V8Isolate* eng, size_t* len) {
    (void)eng; *len = 0; return nullptr;
}

void v8_isolate_set_direct_buffer(V8Isolate* eng, void* buf, size_t len) {
    (void)eng; (void)buf; (void)len;
}

/* ── GC ──────────────────────────────────────────── */

void v8_isolate_gc(V8Isolate* eng) {
    v8::Isolate::Scope iso_scope(eng->isolate);
    v8::HandleScope    handle_scope(eng->isolate);
    eng->isolate->LowMemoryNotification();
}

size_t v8_isolate_heap_used(V8Isolate* eng) {
    v8::Isolate::Scope iso_scope(eng->isolate);
    v8::HandleScope    handle_scope(eng->isolate);
    v8::HeapStatistics stats;
    eng->isolate->GetHeapStatistics(&stats);
    return stats.used_heap_size();
}

size_t v8_isolate_heap_total(V8Isolate* eng) {
    v8::Isolate::Scope iso_scope(eng->isolate);
    v8::HandleScope    handle_scope(eng->isolate);
    v8::HeapStatistics stats;
    eng->isolate->GetHeapStatistics(&stats);
    return stats.total_heap_size();
}

/* ── Snapshot creation ────────────────────────────── */

int v8_create_snapshot(const char* warmup_script, const char* output_path,
                        size_t heap_mb) {
    ensure_v8_init();

    /* Step 1: Create a temporary isolate, run warmup, then snapshot it */
    v8::SnapshotCreator creator;
    v8::Isolate* iso = creator.GetIsolate();
    {
        v8::Isolate::Scope iso_scope(iso);
        v8::HandleScope    handle_scope(iso);
        v8::Local<v8::Context> ctx = v8::Context::New(iso);
        v8::Context::Scope ctx_scope(ctx);

        /* Run warmup scripts */
        if (warmup_script && warmup_script[0]) {
            v8::Local<v8::String> source =
                v8::String::NewFromUtf8(iso, warmup_script).ToLocalChecked();
            v8::Script::Compile(ctx, source).ToLocalChecked()->Run(ctx);
        }
        /* Force GC to compact the heap before snapshotting */
        iso->LowMemoryNotification();
    }

    /* Step 2: Serialize the snapshot */
    v8::StartupData blob = creator.CreateBlob(
        v8::SnapshotCreator::FunctionCodeHandling::kClear);

    /* Step 3: Write to file */
    FILE* f = fopen(output_path, "wb");
    if (!f) return -1;
    fwrite(blob.data, 1, blob.raw_size, f);
    fclose(f);
    delete[] blob.data;
    return 0;
}

/* ── Global variable access (JSON bridge) ─────────── */

char* v8_get_global_json(V8Isolate* eng, const char* name) {
    v8::Isolate* iso = eng->isolate;
    v8::Isolate::Scope iso_scope(iso);
    v8::HandleScope    handle_scope(iso);
    v8::Local<v8::Context> ctx = eng->context.Get(iso);
    v8::Context::Scope ctx_scope(ctx);

    v8::Local<v8::String> key = v8::String::NewFromUtf8(iso, name).ToLocalChecked();
    v8::MaybeLocal<v8::Value> maybe_val = ctx->Global()->Get(ctx, key);
    if (maybe_val.IsEmpty()) return strdup("null");

    v8::Local<v8::Value> val = maybe_val.ToLocalChecked();
    if (val->IsUndefined() || val->IsNull()) return strdup("null");

    /* Use JSON.stringify to serialize */
    v8::Local<v8::Object> json_obj = ctx->Global()->Get(ctx,
        v8::String::NewFromUtf8(iso, "JSON").ToLocalChecked())
        .ToLocalChecked().As<v8::Object>();
    v8::Local<v8::Function> stringify = json_obj->Get(ctx,
        v8::String::NewFromUtf8(iso, "stringify").ToLocalChecked())
        .ToLocalChecked().As<v8::Function>();

    v8::Local<v8::Value> argv[] = { val };
    v8::MaybeLocal<v8::Value> result = stringify->Call(ctx, json_obj, 1, argv);
    if (result.IsEmpty()) return strdup("null");

    v8::String::Utf8Value utf8(iso, result.ToLocalChecked());
    return strdup(*utf8 ? *utf8 : "null");
}

void v8_set_global_json(V8Isolate* eng, const char* name, const char* json) {
    v8::Isolate* iso = eng->isolate;
    v8::Isolate::Scope iso_scope(iso);
    v8::HandleScope    handle_scope(iso);
    v8::Local<v8::Context> ctx = eng->context.Get(iso);
    v8::Context::Scope ctx_scope(ctx);

    /* Use JSON.parse to deserialize */
    v8::Local<v8::Object> json_obj = ctx->Global()->Get(ctx,
        v8::String::NewFromUtf8(iso, "JSON").ToLocalChecked())
        .ToLocalChecked().As<v8::Object>();
    v8::Local<v8::Function> parse = json_obj->Get(ctx,
        v8::String::NewFromUtf8(iso, "parse").ToLocalChecked())
        .ToLocalChecked().As<v8::Function>();

    v8::Local<v8::Value> argv[] = {
        v8::String::NewFromUtf8(iso, json).ToLocalChecked()
    };
    v8::MaybeLocal<v8::Value> val = parse->Call(ctx, json_obj, 1, argv);

    v8::Local<v8::String> key = v8::String::NewFromUtf8(iso, name).ToLocalChecked();
    if (!val.IsEmpty()) {
        ctx->Global()->Set(ctx, key, val.ToLocalChecked());
    }
}

/* ── Callback registry ────────────────────────────── */

#include <map>
#include <string>

struct CallbackEntry {
    nexa_callback_fn fn;
    void*            user_data;
};

static std::map<std::string, CallbackEntry> g_callbacks;

void v8_bind_callback(V8Isolate* eng, const char* name,
                       nexa_callback_fn fn, void* user_data) {
    g_callbacks[name] = {fn, user_data};

    v8::Isolate* iso = eng->isolate;
    v8::Isolate::Scope iso_scope(iso);
    v8::HandleScope    handle_scope(iso);
    v8::Local<v8::Context> ctx = eng->context.Get(iso);
    v8::Context::Scope ctx_scope(ctx);

    /* Create a V8 function that wraps the C callback.
     * Store callback name in `data` — V8 passes it as args.Data(). */
    v8::Local<v8::String> cb_name = v8::String::NewFromUtf8(iso, name).ToLocalChecked();
    std::string key(name);
    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(iso,
        [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            v8::Isolate* iso = args.GetIsolate();
            v8::Local<v8::Context> ctx = iso->GetCurrentContext();
            v8::String::Utf8Value name_utf8(iso, args.Data());
            std::string key(*name_utf8);

            auto it = g_callbacks.find(key);
            if (it == g_callbacks.end()) {
                args.GetReturnValue().Set(v8::Undefined(iso));
                return;
            }

            /* Serialize all JS arguments to JSON array */
            v8::Local<v8::Array> js_args = v8::Array::New(iso, args.Length());
            for (int i = 0; i < args.Length(); i++) {
                js_args->Set(ctx, i, args[i]);
            }

            v8::Local<v8::Object> json_obj = ctx->Global()->Get(ctx,
                v8::String::NewFromUtf8(iso, "JSON").ToLocalChecked())
                .ToLocalChecked().As<v8::Object>();
            v8::Local<v8::Function> stringify = json_obj->Get(ctx,
                v8::String::NewFromUtf8(iso, "stringify").ToLocalChecked())
                .ToLocalChecked().As<v8::Function>();

            v8::Local<v8::Value> sargv[] = { js_args };
            v8::MaybeLocal<v8::Value> json_result = stringify->Call(ctx, json_obj, 1, sargv);

            v8::String::Utf8Value json_utf8(iso,
                json_result.IsEmpty() ? v8::String::Empty(iso) : json_result.ToLocalChecked());

            /* Call C callback */
            char* response = it->second.fn(it->second.user_data, *json_utf8);

            /* Parse response back to V8 value */
            v8::Local<v8::Function> parse = json_obj->Get(ctx,
                v8::String::NewFromUtf8(iso, "parse").ToLocalChecked())
                .ToLocalChecked().As<v8::Function>();
            v8::Local<v8::Value> parv[] = {
                v8::String::NewFromUtf8(iso, response ? response : "null").ToLocalChecked()
            };
            v8::MaybeLocal<v8::Value> ret = parse->Call(ctx, json_obj, 1, parv);
            if (response) free(response);

            args.GetReturnValue().Set(
                ret.IsEmpty() ? v8::Undefined(iso) : ret.ToLocalChecked());
        }, cb_name);

    v8::Local<v8::Function> fn_obj = tpl->GetFunction(ctx).ToLocalChecked();
    fn_obj->SetName(cb_name);
    ctx->Global()->Set(ctx, cb_name, fn_obj);
}
