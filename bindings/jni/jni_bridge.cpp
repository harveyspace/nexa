#include <jni.h>
#include "nexa.h"
#include <cstring>
#include <cstdlib>

/* ── JNI_OnLoad ────────────────────────────────────── */

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    (void)reserved;
    JNIEnv* env;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_8) != JNI_OK) return JNI_ERR;
    return JNI_VERSION_1_8;
}

/* ── NexaPool ──────────────────────────────────────── */

extern "C" JNIEXPORT jlong JNICALL
Java_io_nexa_NexaPool_nativeCreate(JNIEnv*, jclass, jobject) {
    NexaPoolConfig cfg = {};
    cfg.min_isolates = 1; cfg.max_isolates = 1; cfg.per_process_memory_mb = 128;
    return (jlong)(uintptr_t)nexa_pool_create(&cfg);
}

extern "C" JNIEXPORT jlong JNICALL
Java_io_nexa_NexaPool_nativeAcquire(JNIEnv*, jclass, jlong p) {
    return (jlong)(uintptr_t)nexa_pool_acquire((NexaPool*)(uintptr_t)p);
}

extern "C" JNIEXPORT void JNICALL
Java_io_nexa_NexaPool_nativeRelease(JNIEnv*, jclass, jlong p, jlong i) {
    nexa_pool_release((NexaPool*)(uintptr_t)p, (NexaIsolate*)(uintptr_t)i);
}

extern "C" JNIEXPORT void JNICALL
Java_io_nexa_NexaPool_nativeClose(JNIEnv*, jclass, jlong p) {
    nexa_pool_destroy((NexaPool*)(uintptr_t)p);
}

/* ── NexaIsolate ───────────────────────────────────── */

extern "C" JNIEXPORT jstring JNICALL
Java_io_nexa_NexaIsolate_nativeRun(JNIEnv* env, jclass, jlong iso, jstring s) {
    const char* src = env->GetStringUTFChars(s, nullptr);
    NexaResult* r = nexa_isolate_run((NexaIsolate*)(uintptr_t)iso, src);
    env->ReleaseStringUTFChars(s, src);
    jstring ret = env->NewStringUTF(nexa_result_value(r));
    nexa_result_free(r);
    return ret;
}

extern "C" JNIEXPORT jstring JNICALL
Java_io_nexa_NexaIsolate_nativeLoad(JNIEnv* env, jclass, jlong iso, jstring n, jstring s) {
    const char* nm  = env->GetStringUTFChars(n, nullptr);
    const char* src = env->GetStringUTFChars(s, nullptr);
    NexaResult* r = nexa_isolate_load((NexaIsolate*)(uintptr_t)iso, nm, src);
    env->ReleaseStringUTFChars(n, nm); env->ReleaseStringUTFChars(s, src);
    jstring ret = env->NewStringUTF(nexa_result_value(r)); nexa_result_free(r);
    return ret;
}

extern "C" JNIEXPORT jstring JNICALL
Java_io_nexa_NexaIsolate_nativeCall(JNIEnv* env, jclass, jlong iso, jstring f, jstring a) {
    const char* fn = env->GetStringUTFChars(f, nullptr);
    const char* ar = env->GetStringUTFChars(a, nullptr);
    NexaResult* r = nexa_isolate_call((NexaIsolate*)(uintptr_t)iso, fn, ar);
    env->ReleaseStringUTFChars(f, fn); env->ReleaseStringUTFChars(a, ar);
    jstring ret = env->NewStringUTF(nexa_result_value(r)); nexa_result_free(r);
    return ret;
}

/* Stubs */
extern "C" JNIEXPORT jstring JNICALL
Java_io_nexa_NexaIsolate_nativeGetGlobalJson(JNIEnv* env, jclass, jlong, jstring) {
    return env->NewStringUTF("null");
}
extern "C" JNIEXPORT void JNICALL
Java_io_nexa_NexaIsolate_nativeSetGlobalJson(JNIEnv*, jclass, jlong, jstring, jstring) {}
extern "C" JNIEXPORT void JNICALL
Java_io_nexa_NexaIsolate_nativeBindCallback(JNIEnv*, jclass, jlong, jstring) {}
extern "C" JNIEXPORT void JNICALL
Java_io_nexa_NexaIsolate_nativeDestroy(JNIEnv*, jclass, jlong) {}
