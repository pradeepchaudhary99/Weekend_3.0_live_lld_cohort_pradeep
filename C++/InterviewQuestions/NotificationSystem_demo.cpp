/*
FunctionalRequirements:

Users should be able to send Notification
System should be able to support multiple types of channels
    --> Email
    --> SMS
    --> Push
    --> Whatsapp
    --> Slack
Notification System should care about user preference
Notification sending should be asynchronous
Easy to add new notification types/channels


Entities:

User --> Client ---> main --> class Clients
            // SendNotification

Notification
NotificationService
NotificationChannel
UserPreferencesRepository
NotificationChannelFactory
*/

#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

struct Notification {
    std::string recipient;
    std::string title;
    std::string message;
};

struct NotificationChannel {
    virtual void sendNotification(const Notification& notification) = 0;
    virtual ~NotificationChannel() = default;
};

class NotificationDecorator : public NotificationChannel {
protected:
    std::shared_ptr<NotificationChannel> notificationChannel;

public:
    explicit NotificationDecorator(std::shared_ptr<NotificationChannel> channel)
        : notificationChannel(std::move(channel)) {}
};

class RateLimiterDecorator : public NotificationDecorator {
public:
    using NotificationDecorator::NotificationDecorator;

    void sendNotification(const Notification& notification) override {
        // 1000 lines of rate limiting logic will be here
        notificationChannel->sendNotification(notification);
    }
};

class SMSNotification : public NotificationChannel {
public:
    void sendNotification(const Notification& notification) override {}
};

class PushNotification : public NotificationChannel {
public:
    void sendNotification(const Notification& notification) override {}
};

class WhatsappNotification : public NotificationChannel {
public:
    void sendNotification(const Notification& notification) override {}
};

class EmailNotification : public NotificationChannel {
public:
    void sendNotification(const Notification& notification) override {
        throw std::logic_error("sendNotification not implemented");
    }
};

enum class NotificationType { SMS, PUSH, EMAIL, WHATSAPP };

class NotificationFactory {
    std::unordered_map<NotificationType, std::shared_ptr<NotificationChannel>> registry;

public:
    NotificationFactory() {
        registry[NotificationType::EMAIL] = std::make_shared<EmailNotification>();
        registry[NotificationType::SMS] = std::make_shared<EmailNotification>();
        registry[NotificationType::WHATSAPP] = std::make_shared<EmailNotification>();
    }

    std::shared_ptr<NotificationChannel> getInstance(NotificationType type) {
        return registry.at(type);
    }
};

class UserPreferencesRepository {
    std::unordered_map<std::string, std::vector<std::shared_ptr<NotificationChannel>>> userPreferences;

public:
    std::vector<std::shared_ptr<NotificationChannel>> getUserPreference(const std::string& userId) {
        auto it = userPreferences.find(userId);
        if (it == userPreferences.end()) {
            return {};
        }
        return it->second;
    }
};

// Orchestrator, Facade
class NotificationService {
    NotificationFactory factory;
    UserPreferencesRepository userPreferencesRepository;

public:
    void send(const Notification& notification) {
        auto preferredChannels = userPreferencesRepository.getUserPreference(notification.recipient);
        for (auto& channel : preferredChannels) {
            // SMS, Whatsapp, Gmail
            // Async
            std::thread([channel, notification]() {
                channel->sendNotification(notification);
            }).detach();
            // channel->sendNotification(notification); // sync
        }
    }
};

// Applying Decorator Design Pattern:
// Apply Decorator design pattern to enforce Retry Logic, Rate limiting, Formatting
class NotificationClient {
public:
    void sendNotification(NotificationService& notificationService, const Notification& notification) {}
};

class NotificationSystemDemo {};

int main() {
    return 0;
}
