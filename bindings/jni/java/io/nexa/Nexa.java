package io.nexa;

/**
 * Nexa — one-shot entry point.
 *
 * <pre>{@code
 *   String result = Nexa.run("module.exports = 1 + 1;");
 * }</pre>
 */
public final class Nexa {

    static {
        try {
            System.loadLibrary("nexa_jni");
        } catch (UnsatisfiedLinkError e1) {
            try {
                System.load("/usr/local/lib/libnexa_jni.dylib");
            } catch (UnsatisfiedLinkError e2) {
                System.err.println("WARNING: libnexa_jni not found. "
                    + "Build with: cmake --build build && "
                    + "cp build/libnexa_jni.dylib /usr/local/lib/");
            }
        }
    }

    private Nexa() {}

    /** One-shot: create → run → destroy. No pool needed. */
    public static String run(String script) {
        return nativeRun(script);
    }

    /** Standalone isolate. Caller must {@link NexaIsolate#close()}. */
    public static NexaIsolate create(NexaPoolConfig config) {
        long ptr = nativeCreate(config);
        return new NexaIsolate(ptr, true);
    }

    public static String version() {
        return nativeVersion();
    }

    // ── native ──

    private static native String nativeRun(String script);
    private static native long   nativeCreate(NexaPoolConfig config);
    private static native String nativeVersion();
}
