/*
 * test_ipc.cpp — Integration test: spawn nexa_child, communicate via IPC.
 */
#include <gtest/gtest.h>
#include "process_isolate.h"
#include <cstring>
#include <unistd.h>

#ifndef NEXA_CHILD_BINARY
#define NEXA_CHILD_BINARY "../build/nexa_child"
#endif

/* ── Spawn and run ────────────────────────────────── */

TEST(IPCTest, SpawnAndRun) {
    ProcessIsolate* proc = nexa_process_spawn(
        NEXA_CHILD_BINARY, 64, 1024);
    ASSERT_NE(proc, nullptr);
    EXPECT_TRUE(nexa_process_alive(proc));

    char* resp = nexa_process_run(proc, "1 + 1", 5000);
    ASSERT_NE(resp, nullptr);
    /* Response should contain "ok":true */
    EXPECT_TRUE(strstr(resp, "\"ok\":true") != nullptr);
    free(resp);

    nexa_process_kill(proc);
}

/* ── Load + Call ──────────────────────────────────── */

TEST(IPCTest, LoadAndCall) {
    ProcessIsolate* proc = nexa_process_spawn(
        NEXA_CHILD_BINARY, 64, 1024);
    ASSERT_NE(proc, nullptr);

    char* lr = nexa_process_load(proc, "greet",
        "function greet(p) { var o = JSON.parse(p); return 'Hello ' + o.name; }",
        5000);
    ASSERT_NE(lr, nullptr);
    EXPECT_TRUE(strstr(lr, "\"ok\":true") != nullptr);
    free(lr);

    char* cr = nexa_process_call(proc, "greet", "{\"name\":\"World\"}", 5000);
    ASSERT_NE(cr, nullptr);
    EXPECT_TRUE(strstr(cr, "Hello World") != nullptr);
    free(cr);

    nexa_process_kill(proc);
}

/* ── Crash detection ──────────────────────────────── */

TEST(IPCTest, CrashDetection) {
    ProcessIsolate* proc = nexa_process_spawn(
        NEXA_CHILD_BINARY, 64, 1024);
    ASSERT_NE(proc, nullptr);
    EXPECT_TRUE(nexa_process_alive(proc));

    /* Kill the child externally */
    kill(nexa_process_pid(proc), SIGKILL);
    usleep(200000); /* 200ms for OS to reap */

    EXPECT_FALSE(nexa_process_alive(proc));

    /* Sending to dead process should return null */
    char* resp = nexa_process_run(proc, "1+1", 100);
    EXPECT_EQ(resp, nullptr);

    nexa_process_kill(proc);
}

/* ── Stats ────────────────────────────────────────── */

TEST(IPCTest, Stats) {
    ProcessIsolate* proc = nexa_process_spawn(
        NEXA_CHILD_BINARY, 64, 1024);
    ASSERT_NE(proc, nullptr);

    char* stats = nexa_process_stats(proc);
    ASSERT_NE(stats, nullptr);
    EXPECT_TRUE(strstr(stats, "heap_used") != nullptr);
    free(stats);

    nexa_process_kill(proc);
}
