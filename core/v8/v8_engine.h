/*
 * v8_engine.h — Real V8/libnode engine interface.
 *
 * Same interface as stub_engine.h so nexa.cpp can swap between them
 * via a single #include change.
 */
#ifndef NEXA_V8_ENGINE_H
#define NEXA_V8_ENGINE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct V8Isolate V8Isolate;

V8Isolate* v8_isolate_create(size_t heap_mb, size_t stack_kb);
V8Isolate* v8_isolate_create_with_snapshot(size_t heap_mb, size_t stack_kb,
                                            const char* snapshot_path);
void       v8_isolate_destroy(V8Isolate* iso);

char*  v8_isolate_run(V8Isolate* iso, const char* script);
char*  v8_isolate_call(V8Isolate* iso, const char* func, const char* json);
char*  v8_isolate_load(V8Isolate* iso, const char* name, const char* script);

void   v8_isolate_bind(V8Isolate* iso, const char* name, void* fn_table);
void   v8_isolate_set_global(V8Isolate* iso, const char* name, const char* json);

void*  v8_isolate_direct_buffer(V8Isolate* iso, size_t* len);
void   v8_isolate_set_direct_buffer(V8Isolate* iso, void* buf, size_t len);

void   v8_isolate_gc(V8Isolate* iso);
size_t v8_isolate_heap_used(V8Isolate* iso);
size_t v8_isolate_heap_total(V8Isolate* iso);

/* Snapshot: create a .snap file from a warmup script */
int    v8_create_snapshot(const char* warmup_script, const char* output_path,
                          size_t heap_mb);

char*  v8_isolate_run(V8Isolate* iso, const char* script);
char*  v8_isolate_call(V8Isolate* iso, const char* func, const char* json);
char*  v8_isolate_load(V8Isolate* iso, const char* name, const char* script);

void   v8_isolate_bind(V8Isolate* iso, const char* name, void* fn_table);
void   v8_isolate_set_global(V8Isolate* iso, const char* name, const char* json);

void*  v8_isolate_direct_buffer(V8Isolate* iso, size_t* len);
void   v8_isolate_set_direct_buffer(V8Isolate* iso, void* buf, size_t len);

void   v8_isolate_gc(V8Isolate* iso);
size_t v8_isolate_heap_used(V8Isolate* iso);
size_t v8_isolate_heap_total(V8Isolate* iso);

#ifdef __cplusplus
}
#endif

#endif /* NEXA_V8_ENGINE_H */
