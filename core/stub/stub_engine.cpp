/*
 * stub_engine.cpp — Stub JS engine implementation.
 *
 * Parses simple expressions (1+1, JSON.parse, etc.) using a minimal
 * embedded interpreter. Real V8 integration replaces this file.
 */

#include "stub_engine.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>

struct StubIsolate {
    size_t heap_mb;
    size_t stack_kb;
    size_t heap_used;
    char   last_result[4096];
    char   last_error[1024];
};

StubIsolate* stub_isolate_create(size_t heap_mb, size_t stack_kb) {
    auto* iso = (StubIsolate*)calloc(1, sizeof(StubIsolate));
    iso->heap_mb = heap_mb;
    iso->stack_kb = stack_kb;
    return iso;
}

void stub_isolate_destroy(StubIsolate* iso) {
    free(iso);
}

/* Minimal expression evaluator for demo */
static int eval_simple(const char* script, char* out, size_t out_sz) {
    /* Support: "1 + 1", "module.exports = 1 + 1", JSON.parse, etc. */
    int a = 0, b = 0;
    if (sscanf(script, "%d + %d", &a, &b) == 2) {
        snprintf(out, out_sz, "%d", a + b);
        return 0;
    }
    if (sscanf(script, "module.exports = %d + %d", &a, &b) == 2) {
        snprintf(out, out_sz, "%d", a + b);
        return 0;
    }
    if (strstr(script, "JSON.parse")) {
        snprintf(out, out_sz, "{}");
        return 0;
    }
    snprintf(out, out_sz, "\"stub: ok\"");
    return 0;
}

char* stub_isolate_run(StubIsolate* iso, const char* script) {
    char buf[4096];
    eval_simple(script, buf, sizeof(buf));
    iso->heap_used += 1;
    return strdup(buf);
}

char* stub_isolate_call(StubIsolate* iso, const char* func, const char* json) {
    char buf[4096];
    snprintf(buf, sizeof(buf), "\"stub: called %s(%s)\"", func, json);
    return strdup(buf);
}

char* stub_isolate_load(StubIsolate* iso, const char* name, const char* script) {
    iso->heap_used += strlen(script) / 100;
    return strdup("\"stub: loaded\"");
}

void stub_isolate_bind(StubIsolate* iso, const char* name, void* fn_table) {
    (void)iso; (void)name; (void)fn_table;
}

void stub_isolate_set_global(StubIsolate* iso, const char* name, const char* json) {
    (void)iso; (void)name; (void)json;
}

void* stub_isolate_direct_buffer(StubIsolate* iso, size_t* len) {
    (void)iso; *len = 0; return nullptr;
}

void stub_isolate_set_direct_buffer(StubIsolate* iso, void* buf, size_t len) {
    (void)iso; (void)buf; (void)len;
}

void stub_isolate_gc(StubIsolate* iso) {
    iso->heap_used = 0;
}

size_t stub_isolate_heap_used(StubIsolate* iso) {
    return iso->heap_used;
}

size_t stub_isolate_heap_total(StubIsolate* iso) {
    return iso->heap_mb * 1024 * 1024;
}

char* stub_isolate_get_global_json(StubIsolate* iso, const char* name) {
    return strdup("null");
}

void stub_isolate_bind_callback(StubIsolate* iso, const char* name,
                                 nexa_callback_fn fn, void* user_data) {
    (void)iso; (void)name; (void)fn; (void)user_data;
}
