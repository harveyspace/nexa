package io.nexa;

/**
 * Pool configuration. Defaults are safe for typical server workloads.
 *
 * <pre>{@code
 *   NexaPool pool = NexaPool.create(NexaPoolConfig.builder()
 *       .perProcessMemoryMb(128)
 *       .build());
 * }</pre>
 */
public class NexaPoolConfig {

    private String snapshotPath;
    private int    minIsolates     = 4;
    private int    maxIsolates     = 16;
    private int    perProcessMemoryMb = 128;
    private int    idleTimeoutSec  = 300;
    private int    acquireTimeoutMs = 5000;
    private int    isolateMaxUses  = 0;     // 0 = ∞
    private int    isolateMaxAgeSec = 0;
    private double reuseThreshold  = 0.5;
    private double warnThreshold   = 0.8;
    private int    stackSizeKb     = 1024;
    private String[] warmupModules;         // [name, source, name, source, ...]
    private String[] warmupScripts;

    public static NexaPoolConfig create() { return new NexaPoolConfig(); }

    /** Three presets */
    public static NexaPoolConfig lightweight() {
        return create().perProcessMemoryMb(64).minIsolates(2).maxIsolates(8);
    }
    public static NexaPoolConfig standard() {
        return create();
    }
    public static NexaPoolConfig heavy() {
        return create().perProcessMemoryMb(256).minIsolates(4).maxIsolates(32);
    }

    // ── builders ──

    public NexaPoolConfig snapshotPath(String v)        { this.snapshotPath = v; return this; }
    public NexaPoolConfig minIsolates(int v)             { this.minIsolates = v; return this; }
    public NexaPoolConfig maxIsolates(int v)             { this.maxIsolates = v; return this; }
    public NexaPoolConfig perProcessMemoryMb(int v)      { this.perProcessMemoryMb = v; return this; }
    public NexaPoolConfig idleTimeoutSec(int v)          { this.idleTimeoutSec = v; return this; }
    public NexaPoolConfig acquireTimeoutMs(int v)        { this.acquireTimeoutMs = v; return this; }
    public NexaPoolConfig isolateMaxUses(int v)          { this.isolateMaxUses = v; return this; }
    public NexaPoolConfig isolateMaxAgeSec(int v)        { this.isolateMaxAgeSec = v; return this; }
    public NexaPoolConfig reuseThreshold(double v)       { this.reuseThreshold = v; return this; }
    public NexaPoolConfig warnThreshold(double v)        { this.warnThreshold = v; return this; }
    public NexaPoolConfig stackSizeKb(int v)             { this.stackSizeKb = v; return this; }
    public NexaPoolConfig warmupModules(String[] v)      { this.warmupModules = v; return this; }
    public NexaPoolConfig warmupScripts(String[] v)      { this.warmupScripts = v; return this; }

    // ── getters (JNI) ──

    public String getSnapshotPath()      { return snapshotPath; }
    public int    getMinIsolates()       { return minIsolates; }
    public int    getMaxIsolates()       { return maxIsolates; }
    public int    getPerProcessMemoryMb(){ return perProcessMemoryMb; }
    public int    getIdleTimeoutSec()    { return idleTimeoutSec; }
    public int    getAcquireTimeoutMs()  { return acquireTimeoutMs; }
    public int    getIsolateMaxUses()    { return isolateMaxUses; }
    public int    getIsolateMaxAgeSec()  { return isolateMaxAgeSec; }
    public double getReuseThreshold()    { return reuseThreshold; }
    public double getWarnThreshold()     { return warnThreshold; }
    public int    getStackSizeKb()       { return stackSizeKb; }
    public String[] getWarmupModules()   { return warmupModules; }
    public String[] getWarmupScripts()   { return warmupScripts; }
}
