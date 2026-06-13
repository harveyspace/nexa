/*
 * process_isolate.h — Child-process-backed V8 isolate.
 *
 * Replaces in-process V8Isolate when process isolation is enabled.
 * Manages fork+exec of nexa_child, UDS communication, and crash detection.
 */
#ifndef NEXA_PROCESS_ISOLATE_H
#define NEXA_PROCESS_ISOLATE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ProcessIsolate ProcessIsolate;

/* ── Lifecycle ────────────────────────────────────── */

ProcessIsolate* nexa_process_spawn(const char* child_binary_path,
                                    size_t heap_mb, size_t stack_kb);
void            nexa_process_kill(ProcessIsolate* proc);
int             nexa_process_alive(ProcessIsolate* proc);
int             nexa_process_pid(ProcessIsolate* proc);  /* for testing */

/* ── IPC ──────────────────────────────────────────── */

/* Send a JSON command and receive the JSON response.
 * Returns malloc'd string (caller frees). NULL on timeout/disconnect. */
char* nexa_process_send(ProcessIsolate* proc, const char* method_json,
                         int timeout_ms);

/* ── Convenience wrappers ─────────────────────────── */

char* nexa_process_run(ProcessIsolate* proc, const char* script, int timeout_ms);
char* nexa_process_call(ProcessIsolate* proc, const char* func,
                         const char* json_args, int timeout_ms);
char* nexa_process_load(ProcessIsolate* proc, const char* name,
                         const char* script, int timeout_ms);
char* nexa_process_gc(ProcessIsolate* proc);
char* nexa_process_stats(ProcessIsolate* proc);

#ifdef __cplusplus
}
#endif

#endif /* NEXA_PROCESS_ISOLATE_H */
