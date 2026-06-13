package io.nexa.examples;

import io.nexa.*;
import java.util.List;
import java.util.Map;

/**
 * LoadAndCallExample — the recommended execution pattern for production.
 *
 * load():  parse + compile once (at pool startup or script update)
 * call():  execute compiled function (each request, < 1ms overhead)
 * run():   one-shot compile+execute (scripts that change every request)
 *
 * This is the recommended production pattern: load once, call many. 
 */
public class LoadAndCallExample {

    public static void main(String[] args) {
        basicLoadAndCall();
        multiFunction();
        hotReload();
    }

    /* ── Basic Load + Call ──────────────────────── */

    static void basicLoadAndCall() {
        System.out.println("=== Load + Call ===\n");

        NexaPoolConfig config = NexaPoolConfig.create()
            .minIsolates(1).maxIsolates(1).perProcessMemoryMb(64);

        try (NexaPool pool = NexaPool.create(config)) {
            NexaIsolate iso = pool.acquire();
            try {
                // Step 1: Load the script (compile once)
                String script = ""
                    + "function processOrders(params) {\n"
                    + "  var o = JSON.parse(params);\n"
                    + "  return JSON.stringify({\n"
                    + "    count: o.limit,\n"
                    + "    status: o.status,\n"
                    + "    processed: true\n"
                    + "  });\n"
                    + "}\n";

                iso.load("processOrders", script);
                System.out.println("  [load] processOrders compiled\n");

                // Step 2: Call the function (execute, < 1ms)
                String result = iso.call("processOrders",
                    "{\"status\":\"pending\", \"limit\":100}");
                System.out.println("  [call] " + result);

                // Step 3: Call again with different args
                result = iso.call("processOrders",
                    "{\"status\":\"done\", \"limit\":50}");
                System.out.println("  [call] " + result);

                // ── Simple add(a, b) ──
                iso.load("add", ""
                    + "function add(params) {\n"
                    + "  var o = JSON.parse(params);\n"
                    + "  var sum = o.a + o.b;\n"
                    + "  return JSON.stringify({ sum: sum });\n"
                    + "}\n");

                String r = iso.call("add", "{\"a\":1,\"b\":2}");
                System.out.println("\n  add(1, 2) = " + r);
                r = iso.call("add", "{\"a\":10,\"b\":20}");
                System.out.println("  add(10, 20) = " + r);
                r = iso.call("add", "{\"a\":-5,\"b\":8}");
                System.out.println("  add(-5, 8) = " + r);

            } finally {
                pool.release(iso);
            }
        }
    }

    /* ── Multiple functions ──────────────────────── */

    static void multiFunction() {
        System.out.println("\n=== Multiple Functions ===\n");

        NexaPoolConfig config = NexaPoolConfig.create()
            .minIsolates(1).maxIsolates(1).perProcessMemoryMb(64);

        try (NexaPool pool = NexaPool.create(config)) {
            NexaIsolate iso = pool.acquire();
            try {
                // Load order management functions
                iso.load("calculateDiscount", ""
                    + "function calculateDiscount(params) {\n"
                    + "  var o = JSON.parse(params);\n"
                    + "  var discount = o.amount > 100 ? 0.1 : 0.05;\n"
                    + "  return JSON.stringify({ discount: discount });\n"
                    + "}\n");

                iso.load("formatOrder", ""
                    + "function formatOrder(params) {\n"
                    + "  var o = JSON.parse(params);\n"
                    + "  return o.orderId + ': ' + o.itemName;\n"
                    + "}\n");

                System.out.println("  [load] 2 functions compiled\n");

                // Call them independently
                String r1 = iso.call("calculateDiscount",
                    "{\"amount\":150, \"customerLevel\":\"gold\"}");
                System.out.println("  discount: " + r1);

                String r2 = iso.call("formatOrder",
                    "{\"orderId\":\"ORD-001\", \"itemName\":\"Widget\"}");
                System.out.println("  format:   " + r2);

            } finally {
                pool.release(iso);
            }
        }
    }

    /* ── Hot Reload Pattern ──────────────────────── */

    static void hotReload() {
        System.out.println("\n=== Hot Reload ===\n");

        NexaPoolConfig config = NexaPoolConfig.create()
            .minIsolates(1).maxIsolates(1).perProcessMemoryMb(64);

        // Script version 1
        String v1Script = ""
            + "function getVersion() {\n"
            + "  return 'v1.0.0';\n"
            + "}\n";

        // Script version 2 (updated)
        String v2Script = ""
            + "function getVersion() {\n"
            + "  return 'v2.0.0 — improved!';\n"
            + "}\n";

        // Run with v1
        NexaPool pool = NexaPool.create(config);
        NexaIsolate iso = pool.acquire();
        iso.load("getVersion", v1Script);
        System.out.println("  v1: " + iso.call("getVersion", "{}"));
        pool.release(iso);
        pool.close();  // Destroy old pool

        // Hot reload: create new pool with v2
        pool = NexaPool.create(config);
        iso = pool.acquire();
        iso.load("getVersion", v2Script);
        System.out.println("  v2: " + iso.call("getVersion", "{}"));
        pool.release(iso);
        pool.close();
    }
}
