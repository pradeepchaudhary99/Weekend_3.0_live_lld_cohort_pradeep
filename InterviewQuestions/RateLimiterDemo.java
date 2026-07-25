/*
Rate Limiter
------------------------------

Functional Requirements:
    System should limit the number of requests a user can make in a given time
    Support multiple algorithms: Token Bucket, Sliding Window
    Rate limiting should be applied per user (per key)
    Easy to add new algorithms

Non-Functional Requirements:
    Thread-safe: many threads can call isAllowed() for the same/different users concurrently
    Low latency: no global lock across all users
    Extensible: new strategies pluggable via the RateLimiter interface (Strategy pattern)

Entities:
    RateLimiter (Strategy interface)
    TokenBucket / TokenBucketRateLimiter
    SlidingWindow / SlidingWindowRateLimiter
    RateLimiterFactory
*/

import java.util.ArrayDeque;
import java.util.Deque;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;

// ---------- Strategy interface ----------

interface RateLimiter {
    boolean isAllowed(String userId);
}

// ---------- Token Bucket ----------

// One bucket per user. All mutable state is only ever touched under the
// bucket's own monitor, so refill + consume happen as one atomic step.
class TokenBucket {
    private final int capacity;
    private final double refillTokensPerSecond;
    private double tokens;
    private long lastRefillTimeMillis;

    public TokenBucket(int capacity, double refillTokensPerSecond) {
        this.capacity = capacity;
        this.refillTokensPerSecond = refillTokensPerSecond;
        this.tokens = capacity;
        this.lastRefillTimeMillis = System.currentTimeMillis();
    }

    private void refill() {
        long now = System.currentTimeMillis();
        double elapsedSeconds = (now - lastRefillTimeMillis) / 1000.0;
        double eligibleTokens = elapsedSeconds * refillTokensPerSecond;
        if (eligibleTokens > 0) {
            tokens = Math.min(capacity, tokens + eligibleTokens);
            lastRefillTimeMillis = now;
        }
    }

    public synchronized boolean tryConsume() {
        refill();
        if (tokens >= 1.0) {
            tokens -= 1.0;
            return true;
        }
        return false;
    }
}

// Per-user buckets are created lazily and exactly once via computeIfAbsent,
// which is atomic on ConcurrentHashMap -- no explicit locking needed here.
class TokenBucketRateLimiter implements RateLimiter {
    private final Map<String, TokenBucket> buckets = new ConcurrentHashMap<>();
    private final int capacity;
    private final double refillTokensPerSecond;

    public TokenBucketRateLimiter(int capacity, double refillTokensPerSecond) {
        this.capacity = capacity;
        this.refillTokensPerSecond = refillTokensPerSecond;
    }

    @Override
    public boolean isAllowed(String userId) {
        TokenBucket bucket = buckets.computeIfAbsent(
                userId, id -> new TokenBucket(capacity, refillTokensPerSecond));
        return bucket.tryConsume();
    }
}

// ---------- Sliding Window ----------

// One window per user. The deque of request timestamps is only mutated
// under the window's own monitor.
class SlidingWindow {
    private final Deque<Long> timestamps = new ArrayDeque<>();
    private final long windowSizeMillis;
    private final int maxRequestsPerWindow;

    public SlidingWindow(long windowSizeMillis, int maxRequestsPerWindow) {
        this.windowSizeMillis = windowSizeMillis;
        this.maxRequestsPerWindow = maxRequestsPerWindow;
    }

    public synchronized boolean isAllowed() {
        long now = System.currentTimeMillis();
        while (!timestamps.isEmpty() && now - timestamps.peekFirst() > windowSizeMillis) {
            timestamps.pollFirst();
        }
        if (timestamps.size() < maxRequestsPerWindow) {
            timestamps.offerLast(now);
            return true;
        }
        return false;
    }
}

class SlidingWindowRateLimiter implements RateLimiter {
    private final Map<String, SlidingWindow> windows = new ConcurrentHashMap<>();
    private final long windowSizeMillis;
    private final int maxRequestsPerWindow;

    public SlidingWindowRateLimiter(long windowSizeMillis, int maxRequestsPerWindow) {
        this.windowSizeMillis = windowSizeMillis;
        this.maxRequestsPerWindow = maxRequestsPerWindow;
    }

    @Override
    public boolean isAllowed(String userId) {
        SlidingWindow window = windows.computeIfAbsent(
                userId, id -> new SlidingWindow(windowSizeMillis, maxRequestsPerWindow));
        return window.isAllowed();
    }
}

// ---------- Factory (Factory pattern: pick an algorithm without callers knowing the class) ----------

enum RateLimiterType {
    TOKEN_BUCKET, SLIDING_WINDOW
}

class RateLimiterFactory {
    public static RateLimiter create(RateLimiterType type, int limit, long windowOrRefillMillis) {
        switch (type) {
            case TOKEN_BUCKET:
                double refillPerSecond = limit / (windowOrRefillMillis / 1000.0);
                return new TokenBucketRateLimiter(limit, refillPerSecond);
            case SLIDING_WINDOW:
                return new SlidingWindowRateLimiter(windowOrRefillMillis, limit);
            default:
                throw new IllegalArgumentException("Unsupported rate limiter type: " + type);
        }
    }
}

// ---------- Demo / Simulation ----------

public class RateLimiterDemo {

    public static void main(String[] args) throws InterruptedException {
        // 5 requests allowed per 1-second window, refilling continuously.
        RateLimiter rateLimiter = RateLimiterFactory.create(RateLimiterType.TOKEN_BUCKET, 5, 1000);

        int threadCount = 10;
        ExecutorService executor = Executors.newFixedThreadPool(threadCount);
        CountDownLatch latch = new CountDownLatch(threadCount);
        AtomicInteger allowed = new AtomicInteger();
        AtomicInteger rejected = new AtomicInteger();

        String userId = "user-1";
        System.out.println("Firing " + threadCount + " concurrent requests for " + userId
                + " against a token bucket of capacity 5...");

        for (int i = 0; i < threadCount; i++) {
            executor.submit(() -> {
                try {
                    if (rateLimiter.isAllowed(userId)) {
                        allowed.incrementAndGet();
                    } else {
                        rejected.incrementAndGet();
                    }
                } finally {
                    latch.countDown();
                }
            });
        }

        latch.await();
        executor.shutdown();

        System.out.println("Allowed: " + allowed.get() + ", Rejected: " + rejected.get());

        System.out.println("\nSliding window demo (max 3 requests / 1000ms):");
        RateLimiter slidingWindowLimiter =
                RateLimiterFactory.create(RateLimiterType.SLIDING_WINDOW, 3, 1000);
        for (int i = 1; i <= 5; i++) {
            System.out.println("Request " + i + " allowed = " + slidingWindowLimiter.isAllowed("user-2"));
        }
    }
}
