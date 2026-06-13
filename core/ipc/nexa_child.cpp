/*
 * nexa_child.cpp — V8 child process (runs as fork'd subprocess).
 *
 * Reads JSON commands from stdin (or UDS), executes via V8,
 * writes JSON responses to stdout.
 *
 * Usage:
 *   nexa_child [--uds /tmp/nexa-XXXX.sock] [--heap 64] [--stack 1024]
 */
#include <v8.h>
#include <libplatform/libplatform.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../v8/v8_engine.h"
#include "ipc_protocol.h"

static v8::Platform* g_platform = nullptr;

/* ── Init ─────────────────────────────────────────── */

static void init_v8() {
    g_platform = v8::platform::NewDefaultPlatform().release();
    v8::V8::InitializePlatform(g_platform);
    v8::V8::Initialize();
}

/* ── IPC helpers ──────────────────────────────────── */

static uint32_t read_u32(FILE* in) {
    uint8_t buf[4];
    if (fread(buf, 1, 4, in) != 4) return 0;
    return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16)
         | ((uint32_t)buf[2] << 8)  |  (uint32_t)buf[3];
}

static void write_u32(FILE* out, uint32_t v) {
    uint8_t buf[4] = {
        (uint8_t)(v >> 24), (uint8_t)(v >> 16),
        (uint8_t)(v >> 8),  (uint8_t)(v)
    };
    fwrite(buf, 1, 4, out);
}

static char* read_msg(FILE* in) {
    uint32_t len = read_u32(in);
    if (len == 0 || len > 4*1024*1024) return nullptr;
    char* buf = (char*)malloc(len + 1);
    if (fread(buf, 1, len, in) != len) { free(buf); return nullptr; }
    buf[len] = '\0';
    return buf;
}

static void write_msg(FILE* out, const char* json) {
    uint32_t len = (uint32_t)strlen(json);
    write_u32(out, len);
    fwrite(json, 1, len, out);
    fflush(out);
}

/* ── JSON helpers (minimal, no library dependency) ── */

static const char* json_get_str(const char* json, const char* key) {
    /* Quick scan for "key": "..." */
    char search[128];
    snprintf(search, sizeof(search), "\"%s\": \"", key);
    const char* p = strstr(json, search);
    if (!p) return "";
    p += strlen(search);
    static char val[4096];
    int i = 0;
    while (*p && *p != '"' && i < (int)sizeof(val)-1) val[i++] = *p++;
    val[i] = '\0';
    return val;
}

static int json_get_int(const char* json, const char* key) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\": ", key);
    const char* p = strstr(json, search);
    return p ? atoi(p + strlen(search)) : 0;
}

static const char* json_get_method(const char* json) {
    return json_get_str(json, "method");
}

static int json_get_id(const char* json) {
    return json_get_int(json, "id");
}

static const char* json_get_param(const char* json, const char* key) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\": \"", key);
    const char* params = strstr(json, "\"params\": {");
    if (!params) return "";
    const char* p = strstr(params, search);
    if (!p) return "";
    p += strlen(search);
    static char val[4096];
    int i = 0;
    while (*p && *p != '"' && i < (int)sizeof(val)-1) val[i++] = *p++;
    val[i] = '\0';
    return val;
}

static void make_response(char* buf, size_t sz, int id, int ok,
                          const char* value, const char* error) {
    snprintf(buf, sz,
        "{\"id\":%d,\"ok\":%s,\"value\":\"%s\",\"error\":\"%s\"}",
        id, ok ? "true" : "false",
        value ? value : "",
        error ? error : "");
}

static void make_error_response(char* buf, size_t sz, int id,
                                const char* error) {
    make_response(buf, sz, id, 0, "", error);
}

/* ── Main ─────────────────────────────────────────── */

int main(int argc, char** argv) {
    int    heap_mb   = 64;
    int    stack_kb  = 1024;
    const char* uds_path = nullptr;
    const char* snapshot_path = nullptr;

    /* Parse args */
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--heap") && i+1 < argc)  heap_mb  = atoi(argv[++i]);
        if (!strcmp(argv[i], "--stack") && i+1 < argc) stack_kb = atoi(argv[++i]);
        if (!strcmp(argv[i], "--uds") && i+1 < argc)   uds_path = argv[++i];
        if (!strcmp(argv[i], "--snapshot") && i+1 < argc) snapshot_path = argv[++i];
    }

    init_v8();

    /* Create V8 isolate (with optional snapshot) */
    V8Isolate* eng = snapshot_path
        ? v8_isolate_create_with_snapshot(heap_mb, stack_kb, snapshot_path)
        : v8_isolate_create(heap_mb, stack_kb);

    /* I/O: use UDS if provided, otherwise stdin/stdout */
    FILE* in  = stdin;
    FILE* out = stdout;

    if (uds_path) {
        /* Connect back to parent's UDS socket */
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, uds_path, sizeof(addr.sun_path)-1);
        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            in  = fdopen(fd, "r");
            out = fdopen(fd, "w");
        }
    }

    char response[8192];

    /* Event loop */
    for (;;) {
        char* msg = read_msg(in);
        if (!msg) break; /* parent closed pipe → exit */

        const char* method = json_get_method(msg);
        int id = json_get_id(msg);

        if (!strcmp(method, NEXA_METHOD_RUN)) {
            const char* script = json_get_param(msg, "script");
            char* result = v8_isolate_run(eng, script);
            make_response(response, sizeof(response), id, 1, result, "");
            free(result);

        } else if (!strcmp(method, NEXA_METHOD_CALL)) {
            const char* func = json_get_param(msg, "function");
            const char* json_args = json_get_param(msg, "args");
            char* result = v8_isolate_call(eng, func, json_args);
            make_response(response, sizeof(response), id, 1, result, "");
            free(result);

        } else if (!strcmp(method, NEXA_METHOD_LOAD)) {
            const char* name   = json_get_param(msg, "name");
            const char* script = json_get_param(msg, "script");
            v8_isolate_load(eng, name, script);
            make_response(response, sizeof(response), id, 1, "\"loaded\"", "");

        } else if (!strcmp(method, NEXA_METHOD_GC)) {
            v8_isolate_gc(eng);
            make_response(response, sizeof(response), id, 1, "\"ok\"", "");

        } else if (!strcmp(method, NEXA_METHOD_STATS)) {
            char buf[256];
            snprintf(buf, sizeof(buf), "{\"heap_used\":%zu,\"heap_total\":%zu}",
                     v8_isolate_heap_used(eng), v8_isolate_heap_total(eng));
            make_response(response, sizeof(response), id, 1, buf, "");

        } else if (!strcmp(method, NEXA_METHOD_SHUTDOWN)) {
            make_response(response, sizeof(response), id, 1, "\"bye\"", "");
            write_msg(out, response);
            break;

        } else {
            make_error_response(response, sizeof(response), id, "unknown method");
        }

        write_msg(out, response);
        free(msg);
    }

    if (uds_path) { fclose(in); fclose(out); }
    v8_isolate_destroy(eng);
    return 0;
}
