/*
 * isolate_pool.cpp — Pool implementation.
 *
 * Manages a pre-warmed collection of V8 child processes (stub for now).
 * Handles acquire/release, watermark checks, auto-healing, stats.
 */
#include "isolate_pool.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>

/* ── Internal helpers ─────────────────────────────── */

static NexaIsolate* create_isolate(const NexaPoolConfig* cfg, uint32_t id) {
    NexaIsolate* iso = (NexaIsolate*)calloc(1, sizeof(NexaIsolate));
    iso->engine = engine_create(cfg->per_process_memory_mb,
                                      cfg->stack_size_kb ? cfg->stack_size_kb : 1024);
    iso->id = id;
    iso->use_count = 0;
    iso->dead = 0;
    return iso;
}

static void destroy_isolate(NexaIsolate* iso, NexaPool* pool) {
    PoolState* s = (PoolState*)pool;
    s->discard_count++;
    engine_destroy(iso->engine);
    free(iso);
}

static int check_watermark(NexaIsolate* iso, const NexaPoolConfig* cfg) {
    /* Force GC */
    engine_gc(iso->engine);
    size_t used  = engine_heap_used(iso->engine);
    size_t total = engine_heap_total(iso->engine);
    double ratio = total > 0 ? (double)used / total : 0;

    if (ratio > cfg->warn_threshold) return 2;  /* discard */
    if (ratio > cfg->reuse_threshold) return 1; /* pending_discard */
    return 0;                                    /* healthy */
}

/* ── Pool API ─────────────────────────────────────── */

NexaPool* pool_create(const NexaPoolConfig* config) {
    PoolState* s = (PoolState*)calloc(1, sizeof(PoolState));
    memcpy(&s->config, config, sizeof(NexaPoolConfig));

    uint32_t min = config->min_isolates ? config->min_isolates : 4;
    s->capacity = config->max_isolates ? config->max_isolates : 16;
    s->isolates = (NexaIsolate**)calloc(s->capacity, sizeof(NexaIsolate*));

    /* Pre-warm */
    for (uint32_t i = 0; i < min; i++) {
        s->isolates[i] = create_isolate(config, i);
        s->idle_count++;
    }
    return (NexaPool*)s;
}

void pool_destroy(NexaPool* pool) {
    PoolState* s = (PoolState*)pool;
    for (uint32_t i = 0; i < s->capacity; i++) {
        if (s->isolates[i]) {
            engine_destroy(s->isolates[i]->engine);
            free(s->isolates[i]);
        }
    }
    free(s->isolates);
    free(s);
}

/* Find an idle isolate (simple linear scan; TODO: lock-free queue) */
static int find_idle(PoolState* s) {
    for (uint32_t i = 0; i < s->capacity; i++) {
        if (s->isolates[i] && !s->isolates[i]->dead && s->isolates[i]->use_count == 0)
            return (int)i;
    }
    /* Try pending-discard slots */
    for (uint32_t i = 0; i < s->capacity; i++) {
        if (s->isolates[i] && s->isolates[i]->pending_discard && s->isolates[i]->use_count == 0)
            return (int)i;
    }
    return -1;
}

/* Find an empty slot */
static int find_empty(PoolState* s) {
    for (uint32_t i = 0; i < s->capacity; i++) {
        if (!s->isolates[i]) return (int)i;
    }
    return -1;
}

NexaIsolate* pool_acquire(NexaPool* pool) {
    return pool_try_acquire(pool, -1);
}

NexaIsolate* pool_try_acquire(NexaPool* pool, int timeout_ms) {
    PoolState* s = (PoolState*)pool;
    (void)timeout_ms; /* TODO: blocking wait */

    int idx = find_idle(s);
    if (idx >= 0) {
        NexaIsolate* iso = s->isolates[idx];
        iso->use_count = 1;
        s->idle_count--;
        s->active_count++;
        s->total_acquired++;
        return iso;
    }

    /* Expand if possible */
    idx = find_empty(s);
    if (idx >= 0) {
        s->isolates[idx] = create_isolate(&s->config, (uint32_t)idx);
        s->isolates[idx]->use_count = 1;
        s->active_count++;
        s->total_acquired++;
        return s->isolates[idx];
    }

    return nullptr; /* Pool exhausted */
}

void pool_release(NexaPool* pool, NexaIsolate* iso) {
    PoolState* s = (PoolState*)pool;

    iso->use_count++;
    int health = check_watermark(iso, &s->config);

    if (health == 2 || iso->dead) {
        /* Discard */
        destroy_isolate(iso, pool);
        s->active_count--;
        /* Auto-heal: replace */
        int slot = find_empty(s);
        if (slot >= 0) {
            s->isolates[slot] = create_isolate(&s->config, (uint32_t)slot);
            s->idle_count++;
        }
        return;
    }

    iso->pending_discard = (health == 1);
    iso->use_count = 0;
    s->active_count--;
    s->idle_count++;

    /* Forced rotation: max uses */
    if (s->config.isolate_max_uses > 0 && iso->use_count >= s->config.isolate_max_uses) {
        iso->pending_discard = 1;
    }
}

NexaPoolStats pool_get_stats(NexaPool* pool) {
    PoolState* s = (PoolState*)pool;
    NexaPoolStats* st = (NexaPoolStats*)calloc(1, sizeof(NexaPoolStats));
    st->active = s->active_count;
    st->idle  = s->idle_count;
    st->oom_count = s->oom_count;
    st->discard_count = s->discard_count;
    return *st;
}
