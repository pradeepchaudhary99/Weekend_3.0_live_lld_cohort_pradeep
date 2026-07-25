/*
FunctionalRequirements:

Users should be able to send Notification
System should support multiple types of channels
    --> Email
    --> SMS
    --> Push
    --> Whatsapp

Notification System should care about user preference
Notification sending should be asynchronous
Easy to add new notification types/channels
Notifications should be resilient to transient channel failures (retry)
Notifications should be rate-limited per recipient per channel


Non-Functional Requirements:
Thread-safe: preferences and channel state are shared across worker threads
Async: sending never blocks the caller
Extensible: new channels/behaviors pluggable via Factory + Decorator


Entities:

User --> Client ---> main --> class Clients
            // SendNotification

Notification
NotificationService
NotificationChannel
UserPreferencesRepository
NotificationChannelFactory

Applying Decorator Design Pattern:
    RateLimiterDecorator, RetryDecorator wrap any NotificationChannel to add
    cross-cutting behavior without touching the concrete channel classes.
*/

#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct Notification {
    std::string recipient;
    std::string title;
    std::string message;
};

class NotificationDeliveryError : public std::runtime_error {
public:
    explicit NotificationDeliveryError(const std::string& message) : std::runtime_error(message) {}
};

struct NotificationChannel {
    virtual ~NotificationChannel() = default;
    virtual void sendNotification(const Notification& notification) = 0;
};

// ---------- Concrete channels ----------

// Builds the full line before a single cout write, so concurrent
// sends from different threads don't interleave mid-line.
namespace {
void printLine(const std::string& line) {
    std::cout << (line + "\n");
}
}  // namespace

class SMSNotification : public NotificationChannel {
public:
    void sendNotification(const Notification& notification) override {
        printLine("[SMS] to " + notification.recipient + ": " + notification.title);
    }
};

class PushNotification : public NotificationChannel {
public:
    void sendNotification(const Notification& notification) override {
        printLine("[PUSH] to " + notification.recipient + ": " + notification.title);
    }
};

class WhatsappNotification : public NotificationChannel {
public:
    void sendNotification(const Notification& notification) override {
        printLine("[WHATSAPP] to " + notification.recipient + ": " + notification.title);
    }
};

class EmailNotification : public NotificationChannel {
public:
    void sendNotification(const Notification& notification) override {
        printLine("[EMAIL] to " + notification.recipient + ": " + notification.title);
    }
};

// ---------- Decorators (Decorator pattern) ----------

class NotificationDecorator : public NotificationChannel {
protected:
    std::shared_ptr<NotificationChannel> notificationChannel;

public:
    explicit NotificationDecorator(std::shared_ptr<NotificationChannel> channel)
        : notificationChannel(std::move(channel)) {}
};

// Fixed-window counter per recipient: at most `limit` sends per `windowMillis`.
// Thread-safe: a mutex guards each recipient's window state.
class RateLimiterDecorator : public NotificationDecorator {
    struct Window {
        int count = 0;
        std::chrono::steady_clock::time_point windowStart = std::chrono::steady_clock::now();
        std::mutex mutex;
    };

    int limit_;
    long long windowMillis_;
    std::unordered_map<std::string, std::unique_ptr<Window>> windows_;
    std::mutex registryMutex_;

    Window& getWindow(const std::string& recipient) {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = windows_.find(recipient);
        if (it == windows_.end()) {
            it = windows_.emplace(recipient, std::make_unique<Window>()).first;
        }
        return *it->second;
    }

public:
    RateLimiterDecorator(std::shared_ptr<NotificationChannel> channel, int limit, long long windowMillis)
        : NotificationDecorator(std::move(channel)), limit_(limit), windowMillis_(windowMillis) {}

    void sendNotification(const Notification& notification) override {
        Window& window = getWindow(notification.recipient);
        {
            std::lock_guard<std::mutex> lock(window.mutex);
            auto now = std::chrono::steady_clock::now();
            auto elapsedMillis =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - window.windowStart).count();
            if (elapsedMillis > windowMillis_) {
                window.windowStart = now;
                window.count = 0;
            }
            if (++window.count > limit_) {
                printLine("[RateLimited] dropping notification to " + notification.recipient);
                return;
            }
        }
        notificationChannel->sendNotification(notification);
    }
};

// Retries the wrapped channel with a short backoff on transient failures.
class RetryDecorator : public NotificationDecorator {
    int maxAttempts_;
    long long backoffMillis_;

public:
    RetryDecorator(std::shared_ptr<NotificationChannel> channel, int maxAttempts, long long backoffMillis)
        : NotificationDecorator(std::move(channel)), maxAttempts_(maxAttempts), backoffMillis_(backoffMillis) {}

    void sendNotification(const Notification& notification) override {
        for (int attempt = 1; attempt <= maxAttempts_; ++attempt) {
            try {
                notificationChannel->sendNotification(notification);
                return;
            } catch (const NotificationDeliveryError& e) {
                printLine("Attempt " + std::to_string(attempt) + " failed for " + notification.recipient
                          + ": " + e.what());
                std::this_thread::sleep_for(std::chrono::milliseconds(backoffMillis_));
            }
        }
        printLine("Giving up on notification to " + notification.recipient
                  + " after " + std::to_string(maxAttempts_) + " attempts");
    }
};

// ---------- Factory (Factory pattern) ----------

enum class NotificationType { SMS, PUSH, EMAIL, WHATSAPP };

struct NotificationTypeHash {
    size_t operator()(NotificationType type) const { return static_cast<size_t>(type); }
};

class NotificationChannelFactory {
    std::unordered_map<NotificationType, std::shared_ptr<NotificationChannel>, NotificationTypeHash> registry_;

    static std::shared_ptr<NotificationChannel> decorate(std::shared_ptr<NotificationChannel> base) {
        // Every channel gets retry-on-failure and per-recipient rate limiting for free.
        auto rateLimited = std::make_shared<RateLimiterDecorator>(std::move(base), 3, 1000);
        return std::make_shared<RetryDecorator>(std::move(rateLimited), 2, 50);
    }

public:
    NotificationChannelFactory() {
        registry_[NotificationType::EMAIL] = decorate(std::make_shared<EmailNotification>());
        registry_[NotificationType::SMS] = decorate(std::make_shared<SMSNotification>());
        registry_[NotificationType::PUSH] = decorate(std::make_shared<PushNotification>());
        registry_[NotificationType::WHATSAPP] = decorate(std::make_shared<WhatsappNotification>());
    }

    std::shared_ptr<NotificationChannel> getInstance(NotificationType type) const {
        return registry_.at(type);
    }
};

// ---------- User preferences ----------

class UserPreferencesRepository {
    std::unordered_map<std::string, std::unordered_set<NotificationType, NotificationTypeHash>> userPreferences_;
    std::mutex mutex_;

public:
    void setUserPreference(const std::string& userId,
                            const std::unordered_set<NotificationType, NotificationTypeHash>& types) {
        std::lock_guard<std::mutex> lock(mutex_);
        userPreferences_[userId] = types;
    }

    std::unordered_set<NotificationType, NotificationTypeHash> getUserPreference(const std::string& userId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = userPreferences_.find(userId);
        if (it == userPreferences_.end()) {
            return {NotificationType::EMAIL};
        }
        return it->second;
    }
};

// Orchestrator / Facade
class NotificationService {
    NotificationChannelFactory factory_;
    UserPreferencesRepository& userPreferencesRepository_;
    std::vector<std::future<void>> pending_;
    std::mutex pendingMutex_;

public:
    explicit NotificationService(UserPreferencesRepository& userPreferencesRepository)
        : userPreferencesRepository_(userPreferencesRepository) {}

    void send(const Notification& notification) {
        auto preferredTypes = userPreferencesRepository_.getUserPreference(notification.recipient);
        for (auto type : preferredTypes) {
            auto channel = factory_.getInstance(type);
            // Async: fan out to every preferred channel without blocking the caller.
            auto future = std::async(std::launch::async, [channel, notification]() {
                channel->sendNotification(notification);
            });
            std::lock_guard<std::mutex> lock(pendingMutex_);
            pending_.push_back(std::move(future));
        }
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        for (auto& future : pending_) {
            future.wait();
        }
        pending_.clear();
    }
};

class NotificationClient {
public:
    void sendNotification(NotificationService& notificationService, const Notification& notification) {
        notificationService.send(notification);
    }
};

int main() {
    UserPreferencesRepository preferences;
    preferences.setUserPreference("alice@example.com", {NotificationType::EMAIL, NotificationType::SMS});
    preferences.setUserPreference("bob@example.com", {NotificationType::PUSH, NotificationType::WHATSAPP});

    NotificationService service(preferences);
    NotificationClient client;

    std::vector<std::thread> callers;
    for (int i = 0; i < 3; ++i) {
        callers.emplace_back([&service, &client, i]() {
            client.sendNotification(service, Notification{"alice@example.com",
                                                            "Order Update " + std::to_string(i),
                                                            "Your order shipped"});
        });
        callers.emplace_back([&service, &client, i]() {
            client.sendNotification(service, Notification{"bob@example.com",
                                                            "Promo " + std::to_string(i),
                                                            "50% off today"});
        });
    }
    for (auto& t : callers) {
        t.join();
    }

    service.shutdown();
    std::cout << "All notifications dispatched.\n";
    return 0;
}
