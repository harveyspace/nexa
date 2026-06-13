/*
 * engine.h — Engine wrapper. Delegates to stub or V8.
 */
#ifndef NEXA_ENGINE_H
#define NEXA_ENGINE_H

#ifdef NEXA_USE_STUB_ENGINE
  #include "stub_engine.h"
  #define EngineIsolate StubIsolate
  #define engine_create stub_isolate_create
  #define engine_destroy stub_isolate_destroy
  #define engine_run stub_isolate_run
  #define engine_call stub_isolate_call
  #define engine_load stub_isolate_load
  #define engine_bind stub_isolate_bind
  #define engine_set_global stub_isolate_set_global
  #define engine_direct_buffer stub_isolate_direct_buffer
  #define engine_set_direct_buffer stub_isolate_set_direct_buffer
  #define engine_gc stub_isolate_gc
  #define engine_heap_used stub_isolate_heap_used
  #define engine_heap_total stub_isolate_heap_total
  #define engine_get_global_json stub_isolate_get_global_json
  #define engine_bind_callback stub_isolate_bind_callback
#else
  #include "v8_engine.h"
  #define EngineIsolate V8Isolate
  #define engine_create v8_isolate_create
  #define engine_destroy v8_isolate_destroy
  #define engine_run v8_isolate_run
  #define engine_call v8_isolate_call
  #define engine_load v8_isolate_load
  #define engine_bind v8_isolate_bind
  #define engine_set_global v8_isolate_set_global
  #define engine_direct_buffer v8_isolate_direct_buffer
  #define engine_set_direct_buffer v8_isolate_set_direct_buffer
  #define engine_gc v8_isolate_gc
  #define engine_heap_used v8_isolate_heap_used
  #define engine_heap_total v8_isolate_heap_total
  #define engine_get_global_json v8_get_global_json
  #define engine_bind_callback v8_bind_callback
#endif

#endif /* NEXA_ENGINE_H */
