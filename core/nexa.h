/*
 * nexa.h — Nexa C API (stable, v0.1)
 *
 * This is the public C interface. All language bindings (JNI, NAPI, Node.js)
 * are built on top of this header. Backward-compatible after 1.0.
 */

#ifndef NEXA_H
#define NEXA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Opaque types ──────────────────────────────────── */

typedef struct NexaPool      NexaPool;
typedef struct NexaIsolate   NexaIsolate;
typedef struct NexaResult    NexaResult;
typedef struct NexaPoolStats NexaPoolStats;

/* ── Pool config ───────────────────────────────────── */

typedef struct {
    const char*  snapshot_path;        /* NULL = no snapshot                    */
    uint32_t     min_isolates;         /* min idle isolates (default: cores)    */
    uint32_t     max_isolates;         /* max isolates (default: cores * 4)     */
    size_t       per_process_memory_mb;/* total memory per child process        */
    uint32_t     idle_timeout_sec;     /* idle before reclaim (default: 300)    */
    uint32_t     acquire_timeout_ms;   /* acquire timeout (default: 5000)       */
    uint32_t     isolate_max_uses;     /* max uses before forced replace (0=∞)  */
    uint32_t     isolate_max_age_sec;  /* max age before forced replace (0=∞)   */
    double       reuse_threshold;      /* heap < this ratio to reuse (0.5)       */
    double       warn_threshold;       /* heap > this ratio to discard (0.8)     */
    uint32_t     stack_size_kb;        /* V8 thread stack (default: 1024)        */
    const char** warmup_scripts;       /* preload scripts (NULL-terminated)      */
    int          warmup_count;
} NexaPoolConfig;

/* ── Pool lifecycle ────────────────────────────────── */

NexaPool*    nexa_pool_create(const NexaPoolConfig* config);
void         nexa_pool_destroy(NexaPool* pool);
NexaPoolStats nexa_pool_stats(NexaPool* pool);

/* ── Isolate acquire / release ─────────────────────── */

NexaIsolate* nexa_pool_acquire(NexaPool* pool);
NexaIsolate* nexa_pool_try_acquire(NexaPool* pool, int timeout_ms);
void         nexa_pool_release(NexaPool* pool, NexaIsolate* iso);

/* ── One-shot ──────────────────────────────────────── */

NexaResult*  nexa_run(const char* script);

/* ── Execution ─────────────────────────────────────── */

NexaResult*  nexa_isolate_run(NexaIsolate* iso, const char* script);
NexaResult*  nexa_isolate_call(NexaIsolate* iso, const char* function,
                               const char* json_args);
NexaResult*  nexa_isolate_load(NexaIsolate* iso, const char* name,
                               const char* script);

/* ── Module injection ──────────────────────────────── */

typedef struct {
    const char* name;
    void*       fn_table;            /* opaque: JNI GlobalRef */
} NexaModuleBinding;

void nexa_isolate_bind_modules(NexaIsolate* iso,
                               const NexaModuleBinding* modules,
                               int count);

/* ── Context & globals ─────────────────────────────── */

void nexa_isolate_set_global(NexaIsolate* iso, const char* name,
                             const char* json);

/* ── DirectBuffer (zero-copy) ──────────────────────── */

void* nexa_isolate_get_direct_buffer(NexaIsolate* iso, size_t* len);
void  nexa_isolate_set_direct_buffer(NexaIsolate* iso, void* buf, size_t len);

/* ── Result ────────────────────────────────────────── */

int          nexa_result_ok(const NexaResult* r);
const char*  nexa_result_value(const NexaResult* r);
const char*  nexa_result_error(const NexaResult* r);
int          nexa_result_error_code(const NexaResult* r);
void         nexa_result_free(NexaResult* r);

/* ── Stats ─────────────────────────────────────────── */

uint32_t nexa_pool_stats_active(const NexaPoolStats* s);
uint32_t nexa_pool_stats_idle(const NexaPoolStats* s);
uint64_t nexa_pool_stats_oom_count(const NexaPoolStats* s);
uint64_t nexa_pool_stats_discard_count(const NexaPoolStats* s);
void     nexa_pool_stats_free(NexaPoolStats* s);

/* ── Version ───────────────────────────────────────── */

const char* nexa_version(void);

#ifdef __cplusplus
}
#endif

#endif /* NEXA_H */
