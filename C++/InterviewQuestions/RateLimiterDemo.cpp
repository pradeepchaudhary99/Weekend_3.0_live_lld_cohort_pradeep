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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

struct RateLimiter {
    virtual ~RateLimiter() = default;
    virtual bool isAllowed(const std::string& userId) = 0;
};

// One bucket per user. Its own mutex keeps refill + consume atomic.
class TokenBucket {
public:
    TokenBucket(int capacity, double refillTokensPerSecond)
        : capacity_(capacity), refillTokensPerSecond_(refillTokensPerSecond),
          tokens_(static_cast<double>(capacity)), lastRefillTime_(std::chrono::steady_clock::now()) {}

    bool tryConsume() {
        std::lock_guard<std::mutex> lock(mutex_);
        refill();
        if (tokens_ >= 1.0) {
            tokens_ -= 1.0;
            return true;
        }
        return false;
    }

private:
    void refill() {
        auto now = std::chrono::steady_clock::now();
        double elapsedSeconds = std::chrono::duration<double>(now - lastRefillTime_).count();
        double eligibleTokens = elapsedSeconds * refillTokensPerSecond_;
        if (eligibleTokens > 0) {
            tokens_ = std::min(static_cast<double>(capacity_), tokens_ + eligibleTokens);
            lastRefillTime_ = now;
        }
    }

    int capacity_;
    double refillTokensPerSecond_;
    double tokens_;
    std::chrono::steady_clock::time_point lastRefillTime_;
    std::mutex mutex_;
};

// Per-user buckets are created lazily; the registry mutex only guards
// bucket creation, never the (much hotter) consume path.
class TokenBucketRateLimiter : public RateLimiter {
public:
    TokenBucketRateLimiter(int capacity, double refillTokensPerSecond)
        : capacity_(capacity), refillTokensPerSecond_(refillTokensPerSecond) {}

    bool isAllowed(const std::string& userId) override {
        return getOrCreateBucket(userId)->tryConsume();
    }

private:
    std::shared_ptr<TokenBucket> getOrCreateBucket(const std::string& userId) {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = buckets_.find(userId);
        if (it == buckets_.end()) {
            auto bucket = std::make_shared<TokenBucket>(capacity_, refillTokensPerSecond_);
            it = buckets_.emplace(userId, bucket).first;
        }
        return it->second;
    }

    int capacity_;
    double refillTokensPerSecond_;
    std::unordered_map<std::string, std::shared_ptr<TokenBucket>> buckets_;
    std::mutex registryMutex_;
};

// One window per user; its own mutex keeps the timestamp deque consistent.
class SlidingWindow {
public:
    SlidingWindow(long long windowSizeMillis, int maxRequestsPerWindow)
        : windowSizeMillis_(windowSizeMillis), maxRequestsPerWindow_(maxRequestsPerWindow) {}

    bool isAllowed() {
        std::lock_guard<std::mutex> lock(mutex_);
        long long now = nowMillis();
        while (!timestamps_.empty() && now - timestamps_.front() > windowSizeMillis_) {
            timestamps_.pop_front();
        }
        if (static_cast<int>(timestamps_.size()) < maxRequestsPerWindow_) {
            timestamps_.push_back(now);
            return true;
        }
        return false;
    }

private:
    static long long nowMillis() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }

    std::deque<long long> timestamps_;
    long long windowSizeMillis_;
    int maxRequestsPerWindow_;
    std::mutex mutex_;
};

class SlidingWindowRateLimiter : public RateLimiter {
public:
    SlidingWindowRateLimiter(long long windowSizeMillis, int maxRequestsPerWindow)
        : windowSizeMillis_(windowSizeMillis), maxRequestsPerWindow_(maxRequestsPerWindow) {}

    bool isAllowed(const std::string& userId) override {
        return getOrCreateWindow(userId)->isAllowed();
    }

private:
    std::shared_ptr<SlidingWindow> getOrCreateWindow(const std::string& userId) {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = windows_.find(userId);
        if (it == windows_.end()) {
            auto window = std::make_shared<SlidingWindow>(windowSizeMillis_, maxRequestsPerWindow_);
            it = windows_.emplace(userId, window).first;
        }
        return it->second;
    }

    long long windowSizeMillis_;
    int maxRequestsPerWindow_;
    std::unordered_map<std::string, std::shared_ptr<SlidingWindow>> windows_;
    std::mutex registryMutex_;
};

enum class RateLimiterType { TOKEN_BUCKET, SLIDING_WINDOW };

// Factory pattern: callers pick an algorithm without knowing the concrete class.
class RateLimiterFactory {
public:
    static std::unique_ptr<RateLimiter> create(RateLimiterType type, int limit,
                                                 long long windowOrRefillMillis) {
        switch (type) {
            case RateLimiterType::TOKEN_BUCKET: {
                double refillPerSecond = limit / (windowOrRefillMillis / 1000.0);
                return std::make_unique<TokenBucketRateLimiter>(limit, refillPerSecond);
            }
            case RateLimiterType::SLIDING_WINDOW:
                return std::make_unique<SlidingWindowRateLimiter>(windowOrRefillMillis, limit);
        }
        throw std::invalid_argument("Unsupported rate limiter type");
    }
};

int main() {
    // 5 requests allowed per 1-second window, refilling continuously.
    auto rateLimiter = RateLimiterFactory::create(RateLimiterType::TOKEN_BUCKET, 5, 1000);

    const int threadCount = 10;
    const std::string userId = "user-1";
    std::cout << "Firing " << threadCount << " concurrent requests for " << userId
              << " against a token bucket of capacity 5...\n";

    std::atomic<int> allowed{0};
    std::atomic<int> rejected{0};
    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&]() {
            if (rateLimiter->isAllowed(userId)) {
                allowed.fetch_add(1);
            } else {
                rejected.fetch_add(1);
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Allowed: " << allowed.load() << ", Rejected: " << rejected.load() << "\n";

    std::cout << "\nSliding window demo (max 3 requests / 1000ms):\n";
    auto slidingWindowLimiter = RateLimiterFactory::create(RateLimiterType::SLIDING_WINDOW, 3, 1000);
    for (int i = 1; i <= 5; ++i) {
        std::cout << "Request " << i << " allowed = "
                  << (slidingWindowLimiter->isAllowed("user-2") ? "true" : "false") << "\n";
    }

    return 0;
}
