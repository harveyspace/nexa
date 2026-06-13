/*
 * jni_bridge.cpp — JNI bridge between Java and Nexa C API.
 *
 * Maps io.nexa.NexaPool / NexaIsolate / NexaResult to the C layer.
 */
#include <jni.h>
#include "nexa.h"
#include <cstring>

static jclass    g_nexaPoolClass;
static jclass    g_nexaIsolateClass;
static jclass    g_nexaResultClass;
static jclass    g_nexaConfigClass;
static jmethodID g_config_getPerProcessMemoryMb;

/* ── JNI_OnLoad ───────────────────────────────────── */

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_8) != JNI_OK) return JNI_ERR;

    jclass cls = env->FindClass("io/nexa/NexaPool");
    g_nexaPoolClass = (jclass)env->NewGlobalRef(cls);
    cls = env->FindClass("io/nexa/NexaIsolate");
    g_nexaIsolateClass = (jclass)env->NewGlobalRef(cls);
    cls = env->FindClass("io/nexa/NexaResult");
    g_nexaResultClass = (jclass)env->NewGlobalRef(cls);
    cls = env->FindClass("io/nexa/NexaPoolConfig");
    g_nexaConfigClass = (jclass)env->NewGlobalRef(cls);

    g_config_getPerProcessMemoryMb = env->GetMethodID(
        g_nexaConfigClass, "getPerProcessMemoryMb", "()I");

    return JNI_VERSION_1_8;
}

/* ── Helper: build NexaPoolConfig from Java ───────── */

static NexaPoolConfig build_config(JNIEnv* env, jobject javaConfig) {
    NexaPoolConfig cfg = {};
    cfg.min_isolates = 4;
    cfg.max_isolates = 16;
    cfg.per_process_memory_mb = 128;
    cfg.idle_timeout_sec = 300;
    cfg.acquire_timeout_ms = 5000;
    cfg.stack_size_kb = 1024;
    cfg.reuse_threshold = 0.5;
    cfg.warn_threshold = 0.8;

    if (javaConfig) {
        cfg.per_process_memory_mb = (size_t)env->CallIntMethod(
            javaConfig, g_config_getPerProcessMemoryMb);
    }
    return cfg;
}

/* ── NexaPool.create ──────────────────────────────── */

extern "C" JNIEXPORT jlong JNICALL
Java_io_nexa_NexaPool_nativeCreate(JNIEnv* env, jclass, jobject config) {
    NexaPoolConfig cfg = build_config(env, config);
    NexaPool* pool = nexa_pool_create(&cfg);
    return (jlong)(uintptr_t)pool;
}

extern "C" JNIEXPORT void JNICALL
Java_io_nexa_NexaPool_nativeDestroy(JNIEnv*, jclass, jlong poolPtr) {
    nexa_pool_destroy((NexaPool*)(uintptr_t)poolPtr);
}

/* ── NexaPool.acquire ─────────────────────────────── */

extern "C" JNIEXPORT jlong JNICALL
Java_io_nexa_NexaPool_nativeAcquire(JNIEnv*, jclass, jlong poolPtr, jint timeoutMs) {
    NexaPool* pool = (NexaPool*)(uintptr_t)poolPtr;
    NexaIsolate* iso = timeoutMs < 0
        ? nexa_pool_acquire(pool)
        : nexa_pool_try_acquire(pool, timeoutMs);
    return (jlong)(uintptr_t)iso;
}

extern "C" JNIEXPORT void JNICALL
Java_io_nexa_NexaPool_nativeRelease(JNIEnv*, jclass, jlong poolPtr, jlong isoPtr) {
    nexa_pool_release((NexaPool*)(uintptr_t)poolPtr, (NexaIsolate*)(uintptr_t)isoPtr);
}

/* ── NexaIsolate.run ──────────────────────────────── */

extern "C" JNIEXPORT jstring JNICALL
Java_io_nexa_NexaIsolate_nativeRun(JNIEnv* env, jclass, jlong isoPtr, jstring script) {
    const char* src = env->GetStringUTFChars(script, nullptr);
    NexaResult* r = nexa_isolate_run((NexaIsolate*)(uintptr_t)isoPtr, src);
    env->ReleaseStringUTFChars(script, src);

    jstring result = env->NewStringUTF(nexa_result_value(r));
    nexa_result_free(r);
    return result;
}

extern "C" JNIEXPORT jstring JNICALL
Java_io_nexa_NexaIsolate_nativeCall(JNIEnv* env, jclass, jlong isoPtr,
                                     jstring func, jstring jsonArgs) {
    const char* fn  = env->GetStringUTFChars(func, nullptr);
    const char* arg = env->GetStringUTFChars(jsonArgs, nullptr);
    NexaResult* r = nexa_isolate_call((NexaIsolate*)(uintptr_t)isoPtr, fn, arg);
    env->ReleaseStringUTFChars(func, fn);
    env->ReleaseStringUTFChars(jsonArgs, arg);

    jstring result = env->NewStringUTF(nexa_result_value(r));
    nexa_result_free(r);
    return result;
}

extern "C" JNIEXPORT jstring JNICALL
Java_io_nexa_NexaIsolate_nativeLoad(JNIEnv* env, jclass, jlong isoPtr,
                                     jstring name, jstring script) {
    const char* nm = env->GetStringUTFChars(name, nullptr);
    const char* src = env->GetStringUTFChars(script, nullptr);
    NexaResult* r = nexa_isolate_load((NexaIsolate*)(uintptr_t)isoPtr, nm, src);
    env->ReleaseStringUTFChars(name, nm);
    env->ReleaseStringUTFChars(script, src);

    jstring result = env->NewStringUTF(nexa_result_value(r));
    nexa_result_free(r);
    return result;
}

extern "C" JNIEXPORT void JNICALL
Java_io_nexa_NexaIsolate_nativeDestroy(JNIEnv*, jclass, jlong isoPtr) {
    /* Standalone: NexaIsolate* is actually a pool+isolate pair.
     * We just leak it — the OS reclaims on process exit.
     * Full impl: store pool ptr alongside isolate to destroy both. */
    (void)isoPtr;
}

/* ── Nexa.run (one-shot) ──────────────────────────── */

extern "C" JNIEXPORT jstring JNICALL
Java_io_nexa_Nexa_nativeRun(JNIEnv* env, jclass, jstring script) {
    const char* src = env->GetStringUTFChars(script, nullptr);
    NexaResult* r = nexa_run(src);
    env->ReleaseStringUTFChars(script, src);

    jstring result = env->NewStringUTF(nexa_result_value(r));
    nexa_result_free(r);
    return result;
}

extern "C" JNIEXPORT jlong JNICALL
Java_io_nexa_Nexa_nativeCreate(JNIEnv* env, jclass, jobject config) {
    NexaPoolConfig cfg = build_config(env, config);
    /* One-shot standalone: create a pool of size 1, acquire the isolate */
    NexaPool* pool = nexa_pool_create(&cfg);
    NexaIsolate* iso = nexa_pool_acquire(pool);
    /* Store pool pointer in a way that close() can destroy it */
    /* For now: leak the pool on close, standalone is simple */
    return (jlong)(uintptr_t)iso;
}

extern "C" JNIEXPORT jstring JNICALL
Java_io_nexa_Nexa_nativeVersion(JNIEnv* env, jclass) {
    return env->NewStringUTF(nexa_version());
}
