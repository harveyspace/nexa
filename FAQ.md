# FAQ

## General

### Why not just use Nashorn / GraalJS?
Nashorn is deprecated. GraalJS runs JavaScript on GraalVM but does not support the
full npm ecosystem (no `require('lodash')`, no native C++ modules). Nexa embeds
libnode, giving you the real Node.js standard library and npm.

### Why not run a separate Node.js process?
Process separation adds network latency, serialization overhead, and operational
complexity (two processes to deploy, monitor, and debug). Nexa keeps everything
in-process with UDS + shared memory — sub-millisecond latency.

### Why V8 instead of QuickJS?
QuickJS (~200KB) is lightweight but has no JIT, no npm support, and no DevTools.
V8 (~15MB) delivers 50–100× better compute performance, full npm, and Chrome
DevTools. The tradeoff is worth it for server-side workloads.

### Does Nexa support iOS?
No. Apple restricts third-party JIT engines on iOS. This is a V8 limitation.

---

## Performance

### What's the per-call overhead?
~0.3ms for UDS + shared memory IPC (compared to ~0.1ms for in-process JNI).
For typical server workloads (5–50ms total), this is negligible.

### How many concurrent requests can Nexa handle?
A 4-core server with a 16-isolate pool handles ~1000 QPS at p99 < 20ms.
Actual throughput depends on script complexity and injected module latency.

### How do I benchmark Nexa in my environment?
```java
// JMH benchmark example
@Benchmark
public String benchmarkNexa() {
    NexaIsolate iso = pool.acquire();
    String result = iso.run("module.exports = 1 + 1;").getValue();
    pool.release(iso);
    return result;
}
```

---

## Operations

### What happens when a child process OOMs?
The child process is killed by the kernel OOM killer. The pool detects the
disconnected UDS socket and spawns a replacement. In-flight requests to that
process receive a `NexaChildCrashedException`. All other isolates continue
normally.

### How do I upgrade Nexa without downtime?
1. Create a new pool with the new `libnexa.so` version.
2. Route new traffic to the new pool.
3. Drain and destroy the old pool.
Multiple pools can coexist in the same JVM.

### How do I monitor Nexa in production?
Enable Micrometer integration:
```java
NexaPool pool = NexaPool.create(config);
pool.monitor(meterRegistry);  // → /actuator/metrics/nexa.*
```
Key metrics: `nexa.pool.active`, `nexa.pool.oom.total`, `nexa.pool.acquire.latency`.

---

## Development

### How do I debug JS running in Nexa?
Switch to IN_PROCESS mode and enable the inspector:
```java
NexaPoolConfig.builder()
    .isolation(IN_PROCESS)
    .inspectorEnabled(true)
    .build();
```
Then open `chrome://inspect`. Production must use PROCESS isolation.

### Can I use TypeScript?
Yes. Compile TS to JS, provide the source map, and Nexa maps stack traces
back to your `.ts` files. Alternatively, use `ts-node` or `swc` in your
build pipeline — Nexa runs the resulting JS.

### How do I write a custom module?
Implement any Java class, annotate methods with `@NexaExport`, and register:
```java
pool.setupModules(registry -> registry.bind("myModule", new MyModule()));
```
JS: `nexa.myModule.someMethod()`. Synchronous by default; return
`CompletableFuture<T>` for async.

---

## Memory

### How much memory does Nexa use?
One config: `perProcessMemoryMb(128)`. Per child process, Nexa partitions this
across code cache, JIT code, GC heap, buffers, and IPC. 16-isolate pool ≈
2 GB total for 128MB-per-process config. JVM overhead is ~1 MB.

### What if my JS code leaks memory?
Five-layer defense kicks in: hard heap cap, post-use GC + watermark check,
per-script budget, forced rotation after N uses, and auto-healing pool.
Leaking isolates are discarded and replaced automatically.
