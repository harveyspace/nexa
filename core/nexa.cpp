/*
 * nexa.cpp — Core C API implementation (pool + isolate management).
 *
 * Delegates JS execution to the engine layer (stub or V8).
 */
#include "nexa.h"
#include "engine.h"
#include "isolate_pool.h"
#include <cstdlib>
#include <cstring>

/* ── Internal struct definitions ─────────────────── */

struct NexaResult {
    int    ok;
    int    error_code;
    char*  value;      /* allocated inline after struct */
    char*  error;
};

/* ── One-shot ─────────────────────────────────────── */

NexaResult* nexa_run(const char* script) {
    EngineIsolate* iso = engine_create(64, 1024);
    char* val = engine_run(iso, script);
    NexaResult* r = (NexaResult*)calloc(1, sizeof(NexaResult) + strlen(val) + 1);
    r->ok = 1;
    r->value = (char*)(r + 1);
    strcpy(r->value, val);
    free(val);
    engine_destroy(iso);
    return r;
}

/* ── Pool lifecycle ───────────────────────────────── */

NexaPool* nexa_pool_create(const NexaPoolConfig* config) {
    return pool_create(config);              /* → isolate_pool.cpp */
}

void nexa_pool_destroy(NexaPool* pool) {
    pool_destroy(pool);
}

NexaPoolStats nexa_pool_stats(NexaPool* pool) {
    return pool_get_stats(pool);
}

/* ── Acquire / Release ────────────────────────────── */

NexaIsolate* nexa_pool_acquire(NexaPool* pool) {
    return pool_acquire(pool);
}

NexaIsolate* nexa_pool_try_acquire(NexaPool* pool, int timeout_ms) {
    return pool_try_acquire(pool, timeout_ms);
}

void nexa_pool_release(NexaPool* pool, NexaIsolate* iso) {
    pool_release(pool, iso);
}

/* ── Execution ────────────────────────────────────── */

NexaResult* nexa_isolate_run(NexaIsolate* iso, const char* script) {
    char* val = engine_run(iso->engine, script);
    NexaResult* r = (NexaResult*)calloc(1, sizeof(NexaResult) + strlen(val) + 1);
    r->ok = 1;
    r->value = (char*)(r + 1);
    strcpy(r->value, val);
    free(val);
    return r;
}

NexaResult* nexa_isolate_call(NexaIsolate* iso, const char* function,
                               const char* json_args) {
    char* val = engine_call(iso->engine, function, json_args);
    NexaResult* r = (NexaResult*)calloc(1, sizeof(NexaResult) + strlen(val) + 1);
    r->ok = 1;
    r->value = (char*)(r + 1);
    strcpy(r->value, val);
    free(val);
    return r;
}

NexaResult* nexa_isolate_load(NexaIsolate* iso, const char* name,
                               const char* script) {
    char* val = engine_load(iso->engine, name, script);
    NexaResult* r = (NexaResult*)calloc(1, sizeof(NexaResult) + strlen(val) + 1);
    r->ok = 1;
    r->value = (char*)(r + 1);
    strcpy(r->value, val);
    free(val);
    return r;
}

/* ── Module injection ─────────────────────────────── */

void nexa_isolate_bind_modules(NexaIsolate* iso,
                               const NexaModuleBinding* modules, int count) {
    for (int i = 0; i < count; i++) {
        engine_bind(iso->engine, modules[i].name, modules[i].fn_table);
    }
}

/* ── Context ──────────────────────────────────────── */

void nexa_isolate_set_global(NexaIsolate* iso, const char* name,
                             const char* json) {
    engine_set_global(iso->engine, name, json);
}

/* ── DirectBuffer ─────────────────────────────────── */

void* nexa_isolate_get_direct_buffer(NexaIsolate* iso, size_t* len) {
    return engine_direct_buffer(iso->engine, len);
}

void nexa_isolate_set_direct_buffer(NexaIsolate* iso, void* buf, size_t len) {
    engine_set_direct_buffer(iso->engine, buf, len);
}

/* ── Result ───────────────────────────────────────── */

int nexa_result_ok(const NexaResult* r)       { return r->ok; }
const char* nexa_result_value(const NexaResult* r) { return r->value; }
const char* nexa_result_error(const NexaResult* r) { return r->error ? r->error : ""; }
int nexa_result_error_code(const NexaResult* r) { return r->error_code; }
void nexa_result_free(NexaResult* r)          { free(r); }

/* ── Stats ────────────────────────────────────────── */

uint32_t nexa_pool_stats_active(const NexaPoolStats* s)   { return s->active; }
uint32_t nexa_pool_stats_idle(const NexaPoolStats* s)     { return s->idle; }
uint64_t nexa_pool_stats_oom_count(const NexaPoolStats* s){ return s->oom_count; }
uint64_t nexa_pool_stats_discard_count(const NexaPoolStats* s){ return s->discard_count; }
void     nexa_pool_stats_free(NexaPoolStats* s)            { free(s); }

/* ── Version ──────────────────────────────────────── */

const char* nexa_version(void) { return "0.1.0"; }
