package io.nexa;

/**
 * Handle to a single V8 child process.
 *
 * <pre>{@code
 *   // Pool mode:
 *   NexaIsolate iso = pool.acquire();
 *   iso.load("orders", script);
 *   String result = iso.call("processOrders", "{\"limit\":10}");
 *   pool.release(iso);
 *
 *   // Standalone mode:
 *   NexaIsolate iso = Nexa.create(config);
 *   iso.run("1 + 1");
 *   iso.close();
 * }</pre>
 */
public class NexaIsolate implements AutoCloseable {

    long nativePtr;
    private final boolean standalone;

    NexaIsolate(long ptr, boolean standalone) {
        this.nativePtr = ptr;
        this.standalone = standalone;
    }

    /** Execute a JS string (parse + compile + run). */
    public String run(String script) {
        checkAlive();
        return nativeRun(nativePtr, script);
    }

    /** Compile and register a named script for later calls. */
    public String load(String name, String script) {
        checkAlive();
        return nativeLoad(nativePtr, name, script);
    }

    /** Call a previously loaded function with JSON arguments. */
    public String call(String function, String jsonArgs) {
        checkAlive();
        return nativeCall(nativePtr, function, jsonArgs);
    }

    /** Read a global variable as JSON string. */
    public String getGlobalJson(String name) {
        checkAlive();
        return nativeGetGlobalJson(nativePtr, name);
    }

    /** Set a global variable from JSON string. */
    public void setGlobalJson(String name, String json) {
        checkAlive();
        nativeSetGlobalJson(nativePtr, name, json);
    }

    /** Bind a Java callback as a V8 JS function. Call NexaCallback.register() first. */
    public void bindCallback(String name) {
        checkAlive();
        nativeBindCallback(nativePtr, name);
    }

    /** Destroy standalone isolate. Pool isolates are released via pool. */
    @Override
    public void close() {
        if (standalone && nativePtr != 0) {
            nativeDestroy(nativePtr);
            nativePtr = 0;
        }
    }

    private void checkAlive() {
        if (nativePtr == 0) throw new IllegalStateException("Isolate closed");
    }

    // ── native ──

    private static native String nativeRun(long isoPtr, String script);
    private static native String nativeLoad(long isoPtr, String name, String script);
    private static native String nativeCall(long isoPtr, String func, String jsonArgs);
    private static native String nativeGetGlobalJson(long isoPtr, String name);
    private static native void   nativeSetGlobalJson(long isoPtr, String name, String json);
    private static native void   nativeBindCallback(long isoPtr, String name);
    private static native void   nativeDestroy(long isoPtr);
}
