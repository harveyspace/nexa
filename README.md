# Nexa

<p align="center">
  <strong>V8 inside your JVM. JavaScript, anywhere.</strong>
</p>

<p align="center">
  <a href="#license"><img src="https://img.shields.io/badge/license-Apache%202.0-blue.svg" alt="License"></a>
  <a href="#"><img src="https://img.shields.io/badge/platform-linux%20%7C%20android%20%7C%20harmonyos-lightgrey" alt="Platforms"></a>
  <a href="#"><img src="https://img.shields.io/badge/language-c%2B%2B%20%7C%20java%20%7C%20kotlin%20%7C%20node.js-9cf" alt="Languages"></a>
</p>

---

Nexa embeds V8 (via libnode) into a JVM process. Java and Kotlin applications
gain the full power of Node.js — npm ecosystem, TypeScript, Chrome DevTools —
without running a separate process.

---

## Installation

**Requirements**: Java 11+, Linux x64 or ARM64, glibc ≥ 2.28.

### Maven

```xml
<dependency>
    <groupId>io.nexa</groupId>
    <artifactId>nexa-jni</artifactId>
    <version>0.1.0</version>
</dependency>
```

### Gradle

```groovy
implementation 'io.nexa:nexa-jni:0.1.0'
```

The JAR bundles `libnexa.so` for Linux x64 and ARM64. No external V8 or Node.js
installation is required.

### Build from Source

```bash
git clone https://github.com/nexa/nexa.git && cd nexa
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
# Produces: build/libnexa.so, bindings/jni/target/nexa-jni.jar
```

Requires: CMake 3.20+, GCC 11+ or Clang 15+, JDK 11+.

---

## Quick Start

### 5-line Hello World

```java
import io.nexa.*;

// One-shot: create → run → destroy, no pool needed
NexaResult result = Nexa.run("module.exports = 1 + 1;");
System.out.println(result.getValue()); // "2"
```

### One-shot vs Pool vs Standalone

Nexa offers three usage modes, from simplest to most powerful:

| Mode | API | Best for |
|------|-----|---------|
| **One-shot** | `Nexa.run(script)` | Scripts, CLI tools, tests. Spawns a process, runs, destroys it. |
| **Standalone** | `Nexa.create()` → `iso.run()` → `iso.close()` | Infrequent use. Keep an isolate alive across a few calls, then destroy. |
| **Pool** | `NexaPool.create()` → `acquire()` / `release()` | Production servers. Pre-warmed, concurrent, auto-healing. |

```java
// One-shot — simplest, no lifecycle management
NexaResult r = Nexa.run("1 + 1");

// Standalone — manual lifecycle, reuse across calls
NexaIsolate iso = Nexa.create();
iso.run("var x = 1;");
iso.run("x += 1; module.exports = x;"); // 2 — state persists
iso.close();

// Pool — production, concurrent
NexaPool pool = NexaPool.standard();
NexaIsolate i = pool.acquire();
i.run(script);
pool.release(i);
```

### Spring Boot Integration

```java
// 1. Create a pool of pre-warmed V8 isolates
NexaPool pool = NexaPool.create(NexaPoolConfig.builder()
    .minIsolates(4)
    .maxIsolates(16)
    .build());

// 2. Optionally bind Java objects as JS modules
pool.setupModules(registry -> registry
    .bind("db", new MyDatabaseModule(dataSource)));

// 3. Execute JavaScript
NexaIsolate iso = pool.acquire();
try {
    NexaResult result = iso.run("""
        const orders = nexa.db.find("SELECT * FROM orders WHERE status = ?", "pending");
        module.exports = { count: orders.length };
    """);
    System.out.println(result.getValue());
} finally {
    pool.release(iso);
}
```

## Why Nexa

| | Nexa | Nashorn / GraalJS | Standalone Node.js |
|---|---|---|---|
| JS Engine | V8 (libnode) | V8 / GraalVM | V8 |
| **npm ecosystem** | ✅ full | ❌ | ✅ |
| **TypeScript** | ✅ native | ❌ | ✅ |
| **Chrome DevTools** | ✅ | ❌ | ✅ |
| **JVM integration** | ✅ in-process | ✅ in-process | ❌ separate process |
| **Isolate pool** | ✅ | ❌ | ❌ (worker_threads) |
| **Zero-copy JNI** | ✅ | ❌ | — |
| **Snapshot warmup** | ✅ | ❌ | ❌ |

## Architecture

```
┌──────────────────────────────────────────┐
│               JVM Process (常驻)           │
│  ┌──────────────────────────────────────┐ │
│  │      NexaPool — connection pool      │ │
│  └──────────────┬───────────────────────┘ │
└─────────────────┼─────────────────────────┘
                  │ UDS + shared memory
    ┌─────────────┼─────────────┐
    ▼             ▼             ▼
┌────────┐  ┌────────┐  ┌────────┐
│ V8 子进程│  │ V8 子进程│  │ V8 子进程│
│ (15MB) │  │ (15MB) │  │ (15MB) │
└────────┘  └────────┘  └────────┘
    │             │             │
    │ V8 crash → only this child dies
    │ JVM detects disconnect → auto-restart
```

V8 runs in **separate OS processes**, communicating with the JVM via Unix Domain
Sockets and shared memory. A native crash (SIGSEGV) kills only a single child
process — the JVM detects the disconnect and spawns a replacement automatically.

## Features

- **Process Isolation** — V8 runs in child processes. A native crash kills only that child; JVM auto-restarts it.
- **Memory Protection** — One config: `perProcessMemoryMb(128)`. Internal partition prevents any single category from exhausting the process.
- **Execution Guard** — Per-script timeout, heap budget, and stack depth limit. Stuck scripts are terminated; worst case kills only the child process.
- **Isolate Pool** — Pre-warmed V8 isolates, acquire in < 0.5ms. Thread-safe, lock-free.
- **High Concurrency** — Pool scales with CPU cores. Backpressure: BLOCK / TIMEOUT / REJECT. Per-tenant sub-pools.
- **V8 Snapshots** — Serialize warmed-up bytecode and JIT state; cold start < 10ms
- **Zero-copy I/O** — DirectBuffer bridge, no JSON serialization overhead
- **Module Injection** — `@NexaExport` turns any Java object into `nexa.*`. Automatic JNI lifecycle, no manual release required. Numeric precision auto-mapped (`long` → `BigInt`, `BigDecimal` → `string`).
- **Sync & Async** — Blocking calls for simple logic; `CompletableFuture<T>` → `Promise` for I/O
- **npm / TypeScript** — Full libnode, full ecosystem
- **Shared RO Heap** — One copy of Node.js stdlib across all isolates
- **Production Ready** — Graceful shutdown, health checks, Prometheus metrics, OOM isolation, security sandbox
- **Observable** — JS stack traces, error codes, trace IDs across Java ↔ JS, audit logging, Chrome DevTools, heap snapshots

## Performance

| Metric | Target | Conditions |
|---|---|---|
| Isolate acquire | < 0.5 ms | Pool has idle isolates |
| Cold start (snapshot) | < 10 ms | Deserialize from `.snap` |
| Warm execution (`1+1`) | < 1 ms | Pool reuse, including JNI |
| lodash `_.groupBy` (1k items) | < 5 ms | Warmed |
| 1000 QPS, p99 | < 20 ms | 16-isolate pool, 4 cores |
| Memory (16 isolates) | ~1.1 GB | Off-heap; JVM heap +1 MB |

## Module Injection

Nexa ships with **zero built-in modules**. You define what `nexa.*` contains.

```java
// Java — define any POJO
public class DatabaseModule {
    @NexaExport
    public String find(String sql, String... params) {
        return mapper.writeValueAsString(jdbc.queryForList(sql, (Object[]) params));
    }
}

// Register at startup
pool.setupModules(registry -> registry.bind("db", new DatabaseModule(ds)));
```

```javascript
// JS — call directly, no require()
const orders = nexa.db.find("SELECT * FROM orders WHERE status = ?", "pending");
```

### Sync

`@NexaExport` methods are synchronous by default. The V8 thread blocks until
the Java method returns.

```java
@NexaExport
public String find(String sql) { return jdbc.queryForList(sql); }
```

```javascript
const rows = nexa.db.find("SELECT ...");  // blocks, returns immediately
```

### Async

Return `CompletableFuture<T>` and Nexa converts it to a JS Promise. The V8
thread is released back to the pool while Java executes on its own thread.

```java
@NexaExport
public CompletableFuture<String> fetchUserAsync(String id) {
    return webClient.get()
        .uri("/users/{id}", id)
        .retrieve()
        .bodyToMono(String.class)
        .toFuture();                       // WebClient → CompletableFuture
}
```

```javascript
const [orders, user] = await Promise.all([
    nexa.db.findAsync("SELECT * FROM orders"),
    nexa.api.fetchUserAsync("12345"),
]);
```

## C API

```c
// Pool lifecycle
NexaPool*    nexa_pool_create(NexaPoolConfig* config);
void         nexa_pool_destroy(NexaPool* pool);

// Isolate acquire / release
NexaIsolate* nexa_pool_acquire(NexaPool* pool);
void         nexa_pool_release(NexaPool* pool, NexaIsolate* iso);

// Execution
NexaResult*  nexa_run(NexaIsolate* iso, const char* script, const char* path);
NexaResult*  nexa_run_file(NexaIsolate* iso, const char* filepath);

// Module injection
void nexa_register_module(NexaIsolate* iso, const char* name, void* fn_table);

// Zero-copy buffer
void  nexa_set_direct_buffer(NexaIsolate* iso, void* buf, size_t len);
void* nexa_get_direct_buffer(NexaIsolate* iso, size_t* len);

// Result
int          nexa_result_ok(NexaResult* r);
const char*  nexa_result_value(NexaResult* r);
const char*  nexa_result_error(NexaResult* r);
void         nexa_result_free(NexaResult* r);
```

## Memory Model

**One config**: `perProcessMemoryMb(128)`. Nexa internally partitions this across
9 categories to prevent any single one from exhausting the process.

```
128MB default allocation:
  Code cache:  ~15 MB    (JS + npm source)
  JIT code:    ~23 MB    (TurboFan output)
  V8 GC heap:  ~48 MB    (JS objects, V8 GC managed — hard capped)
  Buffers:     ~24 MB    (ArrayBuffer, external strings)
  Bridge:      ~12 MB    (IPC + return values)
  Overhead:    ~6 MB     (OS)
```

For fine-grained control, individual categories can be overridden:

```java
NexaPoolConfig.builder()
    .perProcessMemoryMb(128)
    .isolateHeapMb(96)       // give GC heap more room
    .build();
```

Each child process has a hard cap. Exceeding it OOMs only that process —
the JVM and other isolates are unaffected. Stale processes are auto-replaced.

Three presets ship out of the box:

```java
NexaPoolConfig.lightweight();   // 64 MB  — simple rule engines
NexaPoolConfig.standard();      // 128 MB — typical business logic
NexaPoolConfig.heavy();         // 256 MB — heavy npm workloads
```

## Platform Support

| Platform | Architecture | Binding | Isolation | Memory | Status |
|---|---|---|---|---|---|
| **Linux** | x64, ARM64 | JNI (Java / Kotlin) | Process (child) | 128 MB | Primary |
| **Android** | ARM64 | NDK (Java / Kotlin) | In-process | 24 MB | Secondary |
| **HarmonyOS** | ARM64 | NAPI (ArkTS) | In-process | 24 MB | Secondary |
| iOS | — | — | — | — | Not supported (JIT restriction) |

On Linux, V8 runs in separate child processes for crash isolation. On Android
and HarmonyOS, V8 runs in-process via NDK/NAPI — process isolation is not
available due to platform constraints, but the tighter memory limits and
controlled JS environment keep risk low.

## Roadmap

| Phase | Scope | Duration |
|---|---|---|
| **1. Core + Modules** | Isolate pool, JNI binding, `@NexaExport` | 4 weeks |
| **2. Snapshots** | V8 snapshot serialize/deserialize, shared RO heap | 2 weeks |
| **3. Zero-copy** | DirectBuffer pool, JNI zero-copy, JS Buffer API | 2 weeks |
| **4. Production** | Prometheus metrics, OOM isolation, graceful shutdown | 2 weeks |
| **5. Mobile** | Android NDK cross-compile, HarmonyOS NAPI | Future |

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

[Apache 2.0](LICENSE)
