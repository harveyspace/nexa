/*
 * isolate_pool.h — V8 child-process pool manager.
 */
#ifndef NEXA_ISOLATE_POOL_H
#define NEXA_ISOLATE_POOL_H

#include "nexa.h"
#include "engine.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Internal isolate handle */
struct NexaIsolate {
    EngineIsolate* engine;
    uint32_t     id;
    uint32_t     use_count;
    int          dead;         /* 1 = OOM/crashed, discard on release */
    int          pending_discard;
    void*        uds_fd;       /* TODO: UDS socket (process isolation) */
};

/* Pool state */
typedef struct {
    NexaIsolate** isolates;
    uint32_t      capacity;
    uint32_t      active_count;
    uint32_t      idle_count;
    uint64_t      total_acquired;
    uint64_t      oom_count;
    uint64_t      discard_count;
    NexaPoolConfig config;
} PoolState;

NexaPool*      pool_create(const NexaPoolConfig* config);
void           pool_destroy(NexaPool* pool);
NexaIsolate*   pool_acquire(NexaPool* pool);
NexaIsolate*   pool_try_acquire(NexaPool* pool, int timeout_ms);
void           pool_release(NexaPool* pool, NexaIsolate* iso);
NexaPoolStats  pool_get_stats(NexaPool* pool);

/* Internal struct — defined here so nexa.cpp can access fields */
struct NexaPoolStats {
    uint32_t active;
    uint32_t idle;
    uint64_t oom_count;
    uint64_t discard_count;
};

#ifdef __cplusplus
}
#endif

#endif /* NEXA_ISOLATE_POOL_H */
