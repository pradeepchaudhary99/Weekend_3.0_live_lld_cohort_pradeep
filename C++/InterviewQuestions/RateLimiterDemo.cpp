#include <algorithm>
#include <chrono>
#include <deque>
#include <string>
#include <unordered_map>

class RateLimiter {
public:
    virtual bool isAllowed(const std::string& userId) = 0;
    virtual ~RateLimiter() = default;
};

class TokenBucket {
    int capacity;
    int tokens;
    long long lastRefillTime;
    double refillRate; // 1 token / 3600 seconds

    static long long nowMillis() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    void refill() {
        long long currentTime = nowMillis();
        long long diff = (currentTime - lastRefillTime) / 1000;
        int eligibleTokens = static_cast<int>(diff * refillRate);
        tokens = std::min(capacity, tokens + eligibleTokens);
        lastRefillTime = currentTime;
    }

public:
    explicit TokenBucket(int capacity = 10, double refillRate = 1.0 / 3600)
        : capacity(capacity), tokens(capacity), lastRefillTime(nowMillis()), refillRate(refillRate) {}

    bool checkIfAllowed() {
        refill();
        if (tokens >= 1) {
            tokens--;
            return true;
        }
        return false;
    }
};

class TokenBucketRateLimitingAlgorithm : public RateLimiter {
    std::unordered_map<std::string, TokenBucket> tokenBuckets;

public:
    bool isAllowed(const std::string& userId) override {
        auto it = tokenBuckets.find(userId);
        if (it == tokenBuckets.end()) {
            it = tokenBuckets.emplace(userId, TokenBucket()).first;
        }
        return it->second.checkIfAllowed();
    }
};

// Per User Sliding Windows
class SlidingWindow {
    std::deque<long long> queue;
    long long windowSize; // seconds
    int maximumRequestAllowedInWindow;

    static long long nowSeconds() {
        return std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

public:
    explicit SlidingWindow(long long windowSize = 60, int maximumRequestAllowedInWindow = 5)
        : windowSize(windowSize), maximumRequestAllowedInWindow(maximumRequestAllowedInWindow) {}

    bool isAllowed() {
        long long currentTime = nowSeconds();
        if (static_cast<int>(queue.size()) < maximumRequestAllowedInWindow) {
            queue.push_back(currentTime);
            return true;
        }

        while (!queue.empty() && currentTime - windowSize > queue.front()) {
            queue.pop_front();
        }

        if (static_cast<int>(queue.size()) < maximumRequestAllowedInWindow) {
            queue.push_back(currentTime);
            return true;
        }
        return false;
    }
};

// Q: Removing from front, inserting at the end....
class SlidingWindowStrategy : public RateLimiter {
    std::unordered_map<std::string, SlidingWindow> windows;

public:
    bool isAllowed(const std::string& userId) override {
        auto it = windows.find(userId);
        if (it == windows.end()) {
            it = windows.emplace(userId, SlidingWindow()).first;
        }
        return it->second.isAllowed();
    }
};

class RateLimiterDemo {};

int main() {
    return 0;
}
