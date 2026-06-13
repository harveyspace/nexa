package io.nexa.examples.spring;

import io.nexa.*;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.bind.annotation.*;

import javax.annotation.PreDestroy;

/**
 * SpringBootExample — production-grade Nexa integration.
 *
 * Pattern:
 *   1. Create NexaPool as a @Bean (singleton)
 *   2. Inject into @Service / @Controller
 *   3. acquire → run/load/call → release (in try-finally)
 *   4. close() on @PreDestroy
 *
 * Run: java -Djava.library.path=../../build -jar target/nexa-spring-example.jar
 */
@SpringBootApplication
public class SpringBootExample {
    public static void main(String[] args) {
        SpringApplication.run(SpringBootExample.class, args);
    }
}

/* ── Nexa Configuration ──────────────────────── */

@Configuration
class NexaConfiguration {

    @Bean
    public NexaPool nexaPool() {
        return NexaPool.create(NexaPoolConfig.create()
            .minIsolates(4)
            .maxIsolates(16)
            .perProcessMemoryMb(128)
            .reuseThreshold(0.5)       // discard if heap > 50%
            .warnThreshold(0.8)        // force discard if heap > 80%
            .isolateMaxUses(1000)      // force rotation after 1000 uses
            .acquireTimeoutMs(5000)
            );
    }

    @Bean
    public NexaScriptLoader scriptLoader(NexaPool pool) {
        NexaScriptLoader loader = new NexaScriptLoader(pool);
        loader.loadScripts();
        return loader;
    }
}

/* ── Script Loader ───────────────────────────── */

class NexaScriptLoader {

    private final NexaPool pool;

    NexaScriptLoader(NexaPool pool) { this.pool = pool; }

    void loadScripts() {
        NexaIsolate iso = pool.acquire();
        try {
            // Load order processing script
            iso.load("processOrders", ""
                + "function processOrders(params) {\n"
                + "  var o = JSON.parse(params);\n"
                + "  return JSON.stringify({\n"
                + "    status: 'processed',\n"
                + "    limit: o.limit,\n"
                + "    timestamp: Date.now()\n"
                + "  });\n"
                + "}\n");

            // Load discount calculation
            iso.load("calculateDiscount", ""
                + "function calculateDiscount(params) {\n"
                + "  var o = JSON.parse(params);\n"
                + "  var rate = o.amount > 1000 ? 0.15\n"
                + "           : o.amount > 500  ? 0.10\n"
                + "           : o.amount > 100  ? 0.05\n"
                + "           : 0;\n"
                + "  return JSON.stringify({\n"
                + "    original: o.amount,\n"
                + "    rate: rate,\n"
                + "    final: o.amount * (1 - rate)\n"
                + "  });\n"
                + "}\n");

        } finally {
            pool.release(iso);
        }
    }

    @PreDestroy
    void shutdown() {
        pool.close();
    }
}

/* ── Service ─────────────────────────────────── */

@org.springframework.stereotype.Service
class OrderService {

    private final NexaPool pool;

    OrderService(NexaPool pool) { this.pool = pool; }

    public String processOrders(int limit) {
        NexaIsolate iso = pool.acquire();
        try {
            return iso.call("processOrders",
                "{\"limit\":" + limit + "}");
        } finally {
            pool.release(iso);
        }
    }

    public String calculateDiscount(double amount) {
        NexaIsolate iso = pool.acquire();
        try {
            return iso.call("calculateDiscount",
                "{\"amount\":" + amount + "}");
        } finally {
            pool.release(iso);
        }
    }
}

/* ── Controller ──────────────────────────────── */

@RestController
@RequestMapping("/api/orders")
class OrderController {

    private final OrderService orderService;

    OrderController(OrderService orderService) { this.orderService = orderService; }

    @GetMapping("/process")
    public String process(@RequestParam(defaultValue = "100") int limit) {
        return orderService.processOrders(limit);
    }

    @GetMapping("/discount")
    public String discount(@RequestParam(defaultValue = "500") double amount) {
        return orderService.calculateDiscount(amount);
    }

    @GetMapping("/health")
    public String health() {
        return "{\"status\":\"ok\",\"version\":\"" + Nexa.version() + "\"}";
    }
}
