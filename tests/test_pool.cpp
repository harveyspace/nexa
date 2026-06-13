/*
 * test_pool.cpp — Unit tests for NexaPool.
 */
#include <gtest/gtest.h>
#include "nexa.h"
#include <cstring>

/* ── Pool create / destroy ────────────────────────── */

TEST(PoolTest, CreateAndDestroy) {
    NexaPoolConfig cfg = {};
    cfg.min_isolates = 2;
    cfg.max_isolates = 4;
    cfg.per_process_memory_mb = 64;

    NexaPool* pool = nexa_pool_create(&cfg);
    ASSERT_NE(pool, nullptr);

    NexaPoolStats stats = nexa_pool_stats(pool);
    EXPECT_EQ(nexa_pool_stats_idle(&stats), 2u);
    EXPECT_EQ(nexa_pool_stats_active(&stats), 0u);
    nexa_pool_stats_free(&stats);

    nexa_pool_destroy(pool);
}

/* ── Acquire / Release ────────────────────────────── */

TEST(PoolTest, AcquireAndRelease) {
    NexaPoolConfig cfg = {};
    cfg.min_isolates = 2;
    cfg.max_isolates = 4;
    cfg.per_process_memory_mb = 64;

    NexaPool* pool = nexa_pool_create(&cfg);
    NexaIsolate* iso = nexa_pool_acquire(pool);
    ASSERT_NE(iso, nullptr);

    NexaPoolStats stats = nexa_pool_stats(pool);
    EXPECT_EQ(nexa_pool_stats_active(&stats), 1u);
    EXPECT_EQ(nexa_pool_stats_idle(&stats), 1u);
    nexa_pool_stats_free(&stats);

    nexa_pool_release(pool, iso);

    stats = nexa_pool_stats(pool);
    EXPECT_EQ(nexa_pool_stats_active(&stats), 0u);
    EXPECT_EQ(nexa_pool_stats_idle(&stats), 2u);
    nexa_pool_stats_free(&stats);

    nexa_pool_destroy(pool);
}

/* ── Run script ───────────────────────────────────── */

TEST(PoolTest, RunScript) {
    NexaPoolConfig cfg = {};
    cfg.min_isolates = 1;
    cfg.max_isolates = 1;
    cfg.per_process_memory_mb = 64;

    NexaPool* pool = nexa_pool_create(&cfg);
    NexaIsolate* iso = nexa_pool_acquire(pool);

    NexaResult* r = nexa_isolate_run(iso, "1 + 1");
    ASSERT_NE(r, nullptr);
    EXPECT_TRUE(nexa_result_ok(r));
    EXPECT_STREQ(nexa_result_value(r), "2");
    nexa_result_free(r);

    nexa_pool_release(pool, iso);
    nexa_pool_destroy(pool);
}

/* ── One-shot ─────────────────────────────────────── */

TEST(PoolTest, OneShot) {
    NexaResult* r = nexa_run("3 + 4");
    ASSERT_NE(r, nullptr);
    EXPECT_TRUE(nexa_result_ok(r));
    EXPECT_STREQ(nexa_result_value(r), "7");
    nexa_result_free(r);
}

/* ── Load + Call ──────────────────────────────────── */

TEST(PoolTest, LoadAndCall) {
    NexaPoolConfig cfg = {};
    cfg.min_isolates = 1;
    cfg.max_isolates = 1;
    cfg.per_process_memory_mb = 64;

    NexaPool* pool = nexa_pool_create(&cfg);
    NexaIsolate* iso = nexa_pool_acquire(pool);

    /* Load a function */
    NexaResult* lr = nexa_isolate_load(iso, "add",
        "function add(params) { var o = JSON.parse(params); return o.a + o.b; }");
    ASSERT_TRUE(nexa_result_ok(lr));
    nexa_result_free(lr);

    /* Call it */
    NexaResult* cr = nexa_isolate_call(iso, "add", "{\"a\":10,\"b\":20}");
    ASSERT_TRUE(nexa_result_ok(cr));
    /* V8 returns the number 30 */
    const char* val = nexa_result_value(cr);
    EXPECT_TRUE(strstr(val, "30") != nullptr);
    nexa_result_free(cr);

    nexa_pool_release(pool, iso);
    nexa_pool_destroy(pool);
}

/* ── Pool exhaustion ──────────────────────────────── */

TEST(PoolTest, PoolExhausted) {
    NexaPoolConfig cfg = {};
    cfg.min_isolates = 1;
    cfg.max_isolates = 1;
    cfg.per_process_memory_mb = 64;

    NexaPool* pool = nexa_pool_create(&cfg);
    NexaIsolate* iso1 = nexa_pool_acquire(pool);
    ASSERT_NE(iso1, nullptr);

    /* Pool is full — should return null */
    NexaIsolate* iso2 = nexa_pool_try_acquire(pool, 0);
    EXPECT_EQ(iso2, nullptr);

    nexa_pool_release(pool, iso1);
    nexa_pool_destroy(pool);
}

/* ── Version ──────────────────────────────────────── */

TEST(PoolTest, Version) {
    EXPECT_STREQ(nexa_version(), "0.1.0");
}
