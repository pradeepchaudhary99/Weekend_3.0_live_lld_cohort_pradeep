#include <memory>
#include <stdexcept>

// Product Interface
struct INotification {
    virtual void send() = 0;
    virtual ~INotification() = default;
};

struct SMSNotification : INotification {
    void send() override {
        throw std::runtime_error("Unimplemented method 'send'");
    }
};

struct WhatsAppNotification : INotification {
    void send() override {
        throw std::runtime_error("Unimplemented method 'send'");
    }
};

struct INotificationFactory {
    virtual std::unique_ptr<INotification> getNotification() = 0;
    virtual ~INotificationFactory() = default;
};

struct SMSNotificationFactory : INotificationFactory {
    std::unique_ptr<INotification> getNotification() override {
        return std::make_unique<SMSNotification>();
    }
};

struct WhatsAppNotificationFactory : INotificationFactory {
    std::unique_ptr<INotification> getNotification() override {
        return std::make_unique<WhatsAppNotification>();
    }
};

struct Slack : INotification {
    void send() override {}
};

struct SlackNotificationFactory : INotificationFactory {
    std::unique_ptr<INotification> getNotification() override {
        return std::make_unique<Slack>();
    }
};

class NotificationService {
public:
    void sendNotification(INotificationFactory& factory) {
        auto notification = factory.getNotification();
        notification->send();
    }
};

int main() {
    NotificationService service;
    SMSNotificationFactory factory;
    service.sendNotification(factory);
    return 0;
}
