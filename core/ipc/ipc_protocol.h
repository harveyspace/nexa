/*
 * ipc_protocol.h — Nexa IPC message format.
 *
 * JVM (parent) ↔ V8 child over Unix Domain Socket.
 * All messages are length-prefixed JSON with a 4-byte header.
 */
#ifndef NEXA_IPC_PROTOCOL_H
#define NEXA_IPC_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Wire format ────────────────────────────────────
 *
 * [4 bytes: payload length (network byte order)]
 * [N bytes: JSON payload]
 *
 * JSON structure:
 *   { "id": <int>, "method": "<string>", "params": { ... } }
 *
 * Response:
 *   { "id": <int>, "ok": <bool>, "value": "<string>", "error": "<string>" }
 */

/* ── Constants ────────────────────────────────────── */

#define NEXA_IPC_MAX_MSG    (4 * 1024 * 1024)  /* 4MB max message */
#define NEXA_IPC_TIMEOUT_MS  5000

/* ── Methods (parent → child) ─────────────────────── */

#define NEXA_METHOD_RUN         "run"
#define NEXA_METHOD_CALL        "call"
#define NEXA_METHOD_LOAD        "load"
#define NEXA_METHOD_BIND        "bind"
#define NEXA_METHOD_SET_GLOBAL  "set_global"
#define NEXA_METHOD_GC          "gc"
#define NEXA_METHOD_STATS       "stats"
#define NEXA_METHOD_SHUTDOWN    "shutdown"

/* ── Error codes ──────────────────────────────────── */

#define NEXA_IPC_OK              0
#define NEXA_IPC_ERR_PARSE      -1
#define NEXA_IPC_ERR_EXEC       -2
#define NEXA_IPC_ERR_TIMEOUT    -3
#define NEXA_IPC_ERR_DISCONNECT -4

/* ── Helper: pack length + JSON into buffer ───────── */

/* Returns malloc'd buffer with [len][json]. Caller frees. */
char* nexa_ipc_pack(const char* json, uint32_t* out_len);

/* ── Helper: read one message from fd ─────────────── */

/* Returns malloc'd JSON string (without length prefix), or NULL on error.
 * Sets *error_code on failure. */
char* nexa_ipc_read_msg(int fd, int timeout_ms, int* error_code);

/* ── Helper: send message to fd ───────────────────── */

/* Returns 0 on success, -1 on error. */
int nexa_ipc_send_msg(int fd, const char* json);

#ifdef __cplusplus
}
#endif

#endif /* NEXA_IPC_PROTOCOL_H */
