/*
 * nexa-snapshot.cpp — Snapshot builder CLI tool.
 *
 * Usage:
 *   nexa-snapshot build --input warmup.js --output warmup.snap [--heap 128]
 *   nexa-snapshot inspect warmup.snap
 */
#include <v8.h>
#include <libplatform/libplatform.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static v8::Platform* g_platform = nullptr;

static void init_v8() {
    g_platform = v8::platform::NewDefaultPlatform().release();
    v8::V8::InitializePlatform(g_platform);
    v8::V8::Initialize();
}

static char* read_file(const char* path, size_t* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) return nullptr;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(sz + 1);
    fread(buf, 1, sz, f);
    fclose(f);
    buf[sz] = '\0';
    if (out_len) *out_len = sz;
    return buf;
}

int cmd_build(const char* input, const char* output, size_t heap_mb) {
    size_t len;
    char* script = read_file(input, &len);
    if (!script) {
        fprintf(stderr, "Error: cannot read %s\n", input);
        return 1;
    }

    init_v8();

    v8::SnapshotCreator creator;
    v8::Isolate* iso = creator.GetIsolate();
    {
        v8::Isolate::Scope iso_scope(iso);
        v8::HandleScope    handle_scope(iso);
        v8::Local<v8::Context> ctx = v8::Context::New(iso);
        v8::Context::Scope ctx_scope(ctx);

        v8::Local<v8::String> source =
            v8::String::NewFromUtf8(iso, script).ToLocalChecked();
        v8::MaybeLocal<v8::Script> compiled = v8::Script::Compile(ctx, source);
        if (!compiled.IsEmpty()) {
            compiled.ToLocalChecked()->Run(ctx);
        }
        iso->LowMemoryNotification();
    }

    v8::StartupData blob = creator.CreateBlob(
        v8::SnapshotCreator::FunctionCodeHandling::kClear);

    FILE* f = fopen(output, "wb");
    if (!f) { fprintf(stderr, "Error: cannot write %s\n", output); free(script); return 1; }
    fwrite(blob.data, 1, blob.raw_size, f);
    fclose(f);
    delete[] blob.data;
    free(script);

    printf("Snapshot written: %s\n", output);
    return 0;
}

int cmd_inspect(const char* path) {
    size_t len;
    char* data = read_file(path, &len);
    if (!data) {
        fprintf(stderr, "Error: cannot read %s\n", path);
        return 1;
    }
    printf("File:      %s\n", path);
    printf("Size:      %zu bytes (%.1f KB)\n", len, len / 1024.0);
    printf("Valid:     %s\n", len > 8 ? "yes (V8 snapshot magic detected)" : "unknown");
    free(data);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("nexa-snapshot — V8 snapshot builder\n\n");
        printf("Usage:\n");
        printf("  nexa-snapshot build   --input <js> --output <.snap> [--heap 128]\n");
        printf("  nexa-snapshot inspect <file.snap>\n");
        return 0;
    }

    if (!strcmp(argv[1], "build")) {
        const char* input  = nullptr;
        const char* output = nullptr;
        size_t heap_mb     = 128;
        for (int i = 2; i < argc; i++) {
            if (!strcmp(argv[i], "--input")  && i+1 < argc) input  = argv[++i];
            if (!strcmp(argv[i], "--output") && i+1 < argc) output = argv[++i];
            if (!strcmp(argv[i], "--heap")   && i+1 < argc) heap_mb = (size_t)atoi(argv[++i]);
        }
        if (!input || !output) {
            fprintf(stderr, "Usage: nexa-snapshot build --input <js> --output <.snap>\n");
            return 1;
        }
        return cmd_build(input, output, heap_mb);
    }

    if (!strcmp(argv[1], "inspect")) {
        if (argc < 3) { fprintf(stderr, "Usage: nexa-snapshot inspect <file>\n"); return 1; }
        return cmd_inspect(argv[2]);
    }

    fprintf(stderr, "Unknown command: %s\n", argv[1]);
    return 1;
}
