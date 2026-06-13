/*
 * test_runner.cpp — Minimal test runner (no external dependencies).
 *
 * Build: c++ -std=c++17 test_runner.cpp -L../build -lnexa_core -lnexa_engine -lnode -I../core -o test_runner
 */
#include "nexa.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) void name(); name();
#define ASSERT(cond, msg) do { \
    tests_run++; \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); return; } \
    tests_passed++; \
} while(0)

/* ── Pool tests ───────────────────────────────────── */

void test_pool_create_destroy() {
    NexaPoolConfig cfg = {};
    cfg.min_isolates = 2; cfg.max_isolates = 4;
    cfg.per_process_memory_mb = 64;
    NexaPool* p = nexa_pool_create(&cfg);
    ASSERT(p != nullptr, "pool_create");
    nexa_pool_destroy(p);
}

void test_acquire_release() {
    NexaPoolConfig cfg = {};
    cfg.min_isolates = 1; cfg.max_isolates = 1;
    cfg.per_process_memory_mb = 64;
    NexaPool* p = nexa_pool_create(&cfg);
    NexaIsolate* iso = nexa_pool_acquire(p);
    ASSERT(iso != nullptr, "acquire");
    nexa_pool_release(p, iso);
    nexa_pool_destroy(p);
}

void test_run_script() {
    NexaPoolConfig cfg = {};
    cfg.min_isolates = 1; cfg.max_isolates = 1;
    cfg.per_process_memory_mb = 64;
    NexaPool* p = nexa_pool_create(&cfg);
    NexaIsolate* iso = nexa_pool_acquire(p);
    NexaResult* r = nexa_isolate_run(iso, "1 + 1");
    ASSERT(r && nexa_result_ok(r), "run 1+1");
    ASSERT(strcmp(nexa_result_value(r), "2") == 0, "result is 2");
    nexa_result_free(r);
    nexa_pool_release(p, iso);
    nexa_pool_destroy(p);
}

void test_one_shot() {
    NexaResult* r = nexa_run("3 + 4");
    ASSERT(r && nexa_result_ok(r), "oneshot ok");
    ASSERT(strcmp(nexa_result_value(r), "7") == 0, "oneshot 7");
    nexa_result_free(r);
}

void test_load_and_call() {
#ifdef NEXA_USE_STUB_ENGINE
    return; /* stub engine doesn't support real function calls */
#else
    NexaPoolConfig cfg = {};
    cfg.min_isolates = 1; cfg.max_isolates = 1;
    cfg.per_process_memory_mb = 64;
    NexaPool* p = nexa_pool_create(&cfg);
    NexaIsolate* iso = nexa_pool_acquire(p);
    NexaResult* lr = nexa_isolate_load(iso, "add",
        "function add(p) { var o=JSON.parse(p); return o.a+o.b; }");
    ASSERT(lr && nexa_result_ok(lr), "load");
    nexa_result_free(lr);
    NexaResult* cr = nexa_isolate_call(iso, "add", "{\"a\":10,\"b\":20}");
    ASSERT(cr && nexa_result_ok(cr), "call");
    ASSERT(strstr(nexa_result_value(cr), "30") != nullptr, "result 30");
    nexa_result_free(cr);
    nexa_pool_release(p, iso);
    nexa_pool_destroy(p);
#endif
}

void test_version() {
    ASSERT(strcmp(nexa_version(), "0.1.0") == 0, "version");
}

/* ── Function add(a, b) ──────────────────────────── */

void test_add_function() {
#ifdef NEXA_USE_STUB_ENGINE
    return;
#else
    NexaPoolConfig cfg = {};
    cfg.min_isolates = 1; cfg.max_isolates = 1;
    cfg.per_process_memory_mb = 64;

    NexaPool* p = nexa_pool_create(&cfg);
    NexaIsolate* iso = nexa_pool_acquire(p);

    /* Load the add function:
     *
     *   function add(a, b) {
     *       print("a+b=", a+b);    // side effect — visible in engine log
     *       return a + b;          // return value — captured by caller
     *   }
     *
     * In Nexa/Node.js: print() writes to stdout (captured in child process).
     * console.log() would need the console object injected via bind().
     */

    const char* script = ""
        "function add(params) {\n"
        "  var o = JSON.parse(params);\n"
        "  var result = o.a + o.b;\n"
        "  return JSON.stringify({ sum: result });\n"
        "}\n";

    NexaResult* lr = nexa_isolate_load(iso, "add", script);
    ASSERT(lr && nexa_result_ok(lr), "load add function");
    nexa_result_free(lr);

    /* Test cases */
    struct { int a, b, expected; } cases[] = {
        {1, 2, 3},
        {10, 20, 30},
        {-5, 8, 3},
        {0, 0, 0},
        {999, 1, 1000},
    };

    for (int i = 0; i < 5; i++) {
        char args[64];
        snprintf(args, sizeof(args), "{\"a\":%d,\"b\":%d}", cases[i].a, cases[i].b);
        NexaResult* cr = nexa_isolate_call(iso, "add", args);

        char msg[80];
        snprintf(msg, sizeof(msg), "add(%d,%d) = %d",
                 cases[i].a, cases[i].b, cases[i].expected);
        ASSERT(cr && nexa_result_ok(cr), msg);

        /* Verify the return contains the expected sum */
        char expected[32];
        snprintf(expected, sizeof(expected), "\"sum\":%d", cases[i].expected);
        ASSERT(strstr(nexa_result_value(cr), expected) != nullptr, msg);

        nexa_result_free(cr);
    }

    nexa_pool_release(p, iso);
    nexa_pool_destroy(p);
#endif
}

/* ── Main ─────────────────────────────────────────── */

int main() {
    printf("=== Nexa Tests ===\n\n");

    TEST(test_pool_create_destroy);
    TEST(test_acquire_release);
    TEST(test_run_script);
    TEST(test_one_shot);
    TEST(test_load_and_call);
    TEST(test_add_function);
    TEST(test_version);

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
