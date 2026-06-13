/*
 * stub_engine.h — Stand-in for V8/libnode during early development.
 *
 * Provides just enough to compile and link the core library so that
 * the C API, pool logic, and JNI bindings can be developed and tested
 * before V8 integration.
 */

#ifndef NEXA_STUB_ENGINE_H
#define NEXA_STUB_ENGINE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle (replaces v8::Isolate*) */
typedef struct StubIsolate StubIsolate;

StubIsolate* stub_isolate_create(size_t heap_mb, size_t stack_kb);
void         stub_isolate_destroy(StubIsolate* iso);

/* Run JS source, returns JSON string (caller frees) */
char*  stub_isolate_run(StubIsolate* iso, const char* script);
char*  stub_isolate_call(StubIsolate* iso, const char* func, const char* json);
char*  stub_isolate_load(StubIsolate* iso, const char* name, const char* script);

/* Module injection */
void   stub_isolate_bind(StubIsolate* iso, const char* name, void* fn_table);
void   stub_isolate_set_global(StubIsolate* iso, const char* name, const char* json);

/* Direct buffer */
void*  stub_isolate_direct_buffer(StubIsolate* iso, size_t* len);
void   stub_isolate_set_direct_buffer(StubIsolate* iso, void* buf, size_t len);

/* GC */
void   stub_isolate_gc(StubIsolate* iso);
size_t stub_isolate_heap_used(StubIsolate* iso);
size_t stub_isolate_heap_total(StubIsolate* iso);

#ifdef __cplusplus
}
#endif

#endif /* NEXA_STUB_ENGINE_H */
