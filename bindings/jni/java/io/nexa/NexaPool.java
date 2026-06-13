package io.nexa;

/**
 * Pre-warmed pool of V8 child processes.
 *
 * <pre>{@code
 *   NexaPool pool = NexaPool.standard();
 *   NexaIsolate iso = pool.acquire();
 *   try { iso.run("1 + 1"); }
 *   finally { pool.release(iso); }
 * }</pre>
 */
public class NexaPool implements AutoCloseable {

    static {
        try {
            System.loadLibrary("nexa_jni");
        } catch (UnsatisfiedLinkError e1) {
            try {
                System.load("/usr/local/lib/libnexa_jni.dylib");
            } catch (UnsatisfiedLinkError e2) {
                System.err.println("WARNING: libnexa_jni not found. "
                    + "Build with: cmake --build build && cp build/libnexa_jni.dylib /usr/local/lib/");
            }
        }
    }

    private final long nativePtr;
    private final NexaPoolConfig config;

    NexaPool(long ptr, NexaPoolConfig config) {
        this.nativePtr = ptr;
        this.config = config;
    }

    /** Create from config. */
    public static NexaPool create(NexaPoolConfig config) {
        return new NexaPool(nativeCreate(config), config);
    }

    /** Presets */
    public static NexaPool lightweight() { return create(NexaPoolConfig.lightweight()); }
    public static NexaPool standard()    { return create(NexaPoolConfig.standard()); }
    public static NexaPool heavy()       { return create(NexaPoolConfig.heavy()); }

    /** Acquire an isolate (blocks until available). */
    public NexaIsolate acquire() {
        return new NexaIsolate(nativeAcquire(nativePtr, -1), false);
    }

    /** Acquire with timeout. Returns null if exhausted. */
    public NexaIsolate acquire(int timeoutMs) {
        long ptr = nativeAcquire(nativePtr, timeoutMs);
        return ptr != 0 ? new NexaIsolate(ptr, false) : null;
    }

    /** Return isolate to pool. */
    public void release(NexaIsolate iso) {
        nativeRelease(nativePtr, iso.nativePtr);
        iso.nativePtr = 0;
    }

    /** Destroy pool and all child processes. */
    @Override
    public void close() {
        nativeDestroy(nativePtr);
    }

    @Override
    public String toString() {
        return "NexaPool[isolates=" + config.getMinIsolates() + ".." + config.getMaxIsolates()
            + " memory=" + config.getPerProcessMemoryMb() + "MB reuse="
            + config.getReuseThreshold() + " warn=" + config.getWarnThreshold() + "]";
    }

    // ── native ──

    private static native long nativeCreate(NexaPoolConfig config);
    private static native void nativeDestroy(long poolPtr);
    private static native long nativeAcquire(long poolPtr, int timeoutMs);
    private static native void nativeRelease(long poolPtr, long isoPtr);
}
