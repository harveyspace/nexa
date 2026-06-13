package io.nexa.examples;

import io.nexa.*;

/**
 * ModuleInjectionExample — demonstrates @NexaExport pattern.
 *
 * The @NexaExport annotation is defined in the Nexa Java API. Any Java class
 * can have its methods exposed to JS via nexa.moduleName.methodName().
 *
 * In this example, we simulate a DatabaseModule and a CacheModule being
 * injected into the JS runtime.
 *
 * ── Architecture ──
 *
 *   JS (V8 child process):
 *     nexa.db.find("SELECT ...")
 *     nexa.cache.get("key")
 *
 *   Java (@NexaExport):
 *     class DatabaseModule  → exposed as nexa.db
 *     class CacheModule     → exposed as nexa.cache
 */
public class ModuleInjectionExample {

    public static void main(String[] args) {
        poolMode();
    }

    /* ── Pool mode with module injection ──────────── */

    static void poolMode() {
        System.out.println("=== Module Injection (Pool) ===\n");

        NexaPoolConfig config = NexaPoolConfig.create()
            .minIsolates(2)
            .maxIsolates(4)
            .perProcessMemoryMb(128)
            ;

        try (NexaPool pool = NexaPool.create(config)) {

            // Register modules at pool creation time.
            // In real code, these would be actual @NexaExport-annotated classes.
            // For this example, we show the registration pattern.
            //
            // pool.setupModules(registry -> registry
            //     .bind("db",    new DatabaseModule(dataSource))
            //     .bind("cache", new CacheModule(redisTemplate))
            //     .bind("logger", new LoggerModule()));

            NexaIsolate iso = pool.acquire();
            try {
                // JS calls injected modules:
                // nexa.db.find("SELECT * FROM orders WHERE status = ?", "pending")
                // nexa.cache.get("session:abc123")
                //
                // Each @NexaExport method call goes:
                //   JS nexa.db.find(args)
                //     → Nexa IPC bridge
                //       → JNI method invocation
                //         → DatabaseModule.find(args)
                //           ← return value
                //         ← JNI
                //       ← IPC
                //   ← JS result

                String script = ""
                    + "// Simulate using injected modules\n"
                    + "var orders = { count: 42, status: 'ok' };\n"
                    + "module.exports = JSON.stringify(orders);\n";

                String result = iso.run(script);
                System.out.println("  result: " + result);

            } finally {
                pool.release(iso);
            }
        }
    }

    /* ── Sample module definitions ────────────────── */

    /**
     * Example: Database module injected as nexa.db.
     *
     * <pre>{@code
     * @NexaExport
     * public class DatabaseModule {
     *     private final DataSource ds;
     *
     *     public String find(String sql, String... params) {
     *         // Run query, return JSON
     *         return jdbcTemplate.queryForList(sql, params).toString();
     *     }
     *
     *     @NexaExport(precision = Precision.STRING)
     *     public long insert(String table, String jsonRow) {
     *         // Returns long ID → auto-converted to String for JS
     *         return jdbcTemplate.update(...);
     *     }
     *
     *     public CompletableFuture<String> findAsync(String sql) {
     *         // Async → JS Promise
     *         return CompletableFuture.supplyAsync(() -> find(sql));
     *     }
     * }
     * }</pre>
     */
    @SuppressWarnings("unused")
    static class DatabaseModule {
        // Stub — real implementation would be @NexaExport annotated
    }

    /**
     * Example: Cache module injected as nexa.cache.
     *
     * <pre>{@code
     * @NexaExport
     * public class CacheModule {
     *     private final StringRedisTemplate redis;
     *
     *     public String get(String key) {
     *         return redis.opsForValue().get(key);
     *     }
     *
     *     public void set(String key, String value, int ttlSeconds) {
     *         redis.opsForValue().set(key, value, Duration.ofSeconds(ttlSeconds));
     *     }
     * }
     * }</pre>
     */
    @SuppressWarnings("unused")
    static class CacheModule {
        // Stub
    }
}
