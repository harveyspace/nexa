package io.nexa.examples;

import io.nexa.*;

/**
 * BasicExamples — demonstrates the three fundamental usage modes of Nexa.
 *
 * 1. One-shot:   Nexa.run(script)          — simplest, use-once
 * 2. Standalone: Nexa.create(config)        — long-lived single isolate
 * 3. Pool:       NexaPool.create(config)    — production, multi-tenant
 */
public class BasicExamples {

    public static void main(String[] args) {
        oneShot();
        standalone();
        pool();
        presets();
    }

    /* ── 1. One-shot ─────────────────────────────── */

    static void oneShot() {
        System.out.println("=== One-shot ===");

        // Parse + compile + execute + destroy. No pool needed.
        String result = Nexa.run("module.exports = 1 + 1;");
        System.out.println("  1 + 1 = " + result);

        // More complex
        result = Nexa.run(""
            + "var items = [1, 2, 3, 4, 5];"
            + "module.exports = items.map(function(x) { return x * 2; });"
        );
        System.out.println("  map x2 = " + result);

        // One-shot creates a temporary isolate each call — fine for
        // occasional use, not for high-throughput.
    }

    /* ── 2. Standalone ───────────────────────────── */

    static void standalone() {
        System.out.println("\n=== Standalone ===");

        // Create a single isolate that lives for multiple calls.
        NexaPoolConfig config = NexaPoolConfig.create()
            .perProcessMemoryMb(64)
            ;

        try (NexaIsolate iso = Nexa.create(config)) {
            // State persists across calls
            iso.run("var counter = 0;");
            String r1 = iso.run("counter++; module.exports = counter;");
            String r2 = iso.run("counter++; module.exports = counter;");
            System.out.println("  counter: " + r1 + " → " + r2);
        }
        // iso.close() called automatically via try-with-resources
    }

    /* ── 3. Pool ─────────────────────────────────── */

    static void pool() {
        System.out.println("\n=== Pool ===");

        // Pool pre-warms isolates. Safe for production concurrency.
        NexaPoolConfig config = NexaPoolConfig.create()
            .minIsolates(2)
            .maxIsolates(4)
            .perProcessMemoryMb(128)
            .acquireTimeoutMs(5000)
            ;

        try (NexaPool pool = NexaPool.create(config)) {

            // Acquire from pool (blocks until available)
            NexaIsolate iso = pool.acquire();
            try {
                String result = iso.run("module.exports = Math.random();");
                System.out.println("  random = " + result);
            } finally {
                pool.release(iso);  // ALWAYS release
            }

            // Try-acquire with timeout — returns null if exhausted
            NexaIsolate iso2 = pool.acquire(100);
            if (iso2 != null) {
                try {
                    System.out.println("  acquired with timeout");
                } finally {
                    pool.release(iso2);
                }
            }

        } // pool.close() → destroy all child processes
    }

    /* ── Presets ──────────────────────────────────── */

    static void presets() {
        System.out.println("\n=== Presets ===");

        // Three built-in presets
        NexaPool light  = NexaPool.lightweight();  // 64MB, 2-8 isolates
        NexaPool std    = NexaPool.standard();     // 128MB, 4-16 isolates
        NexaPool heavy  = NexaPool.heavy();        // 256MB, 4-32 isolates

        System.out.println("  lightweight: " + light);
        System.out.println("  standard:    " + std);
        System.out.println("  heavy:       " + heavy);

        light.close();
        std.close();
        heavy.close();
    }
}
