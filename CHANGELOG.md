# Changelog

All notable changes to Nexa are documented in this file.

## [Unreleased]

### Added
- Initial public release.
- V8 (libnode) embedded in JVM via child-process isolation.
- Isolate pool with acquire/release, auto-healing, and five-layer memory protection.
- `@NexaExport` annotation for Java-to-JS module injection. Automatic JNI lifecycle (no manual release). Numeric precision auto-mapping (`long` → `BigInt`, `BigDecimal` → `string`).
- Synchronous and asynchronous (`CompletableFuture<T>` → `Promise`) invocation.
- One-shot, standalone, and pool usage modes.
- V8 snapshot warmup (`nexa-snapshot` CLI).
- Zero-copy JNI bridge via DirectBuffer and shared memory.
- Chrome DevTools inspector support (IN_PROCESS mode).
- Prometheus / Micrometer metrics, health/readiness checks.
- Pool event hooks (HIGH_WATERMARK, OOM, DISCARD_RATE, etc.).
- Pre-built presets: `lightweight()`, `standard()`, `heavy()`.
- Multi-pool coexistence in the same JVM.
- Per-tenant sub-pool isolation.
- Execution guard: timeout, heap budget, stack size limit.
- Crash dump on isolate failure (heap snapshot, script, audit log).

### Platform Support
- Linux x64, ARM64 (primary).
- Android ARM64 (secondary, planned).
- HarmonyOS ARM64 (secondary, planned).
- iOS: not supported (JIT restriction).
