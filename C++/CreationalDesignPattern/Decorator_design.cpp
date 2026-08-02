#include <iostream>
#include <memory>
#include <string>

// Product
struct INotification {
    virtual void send(const std::string& message) = 0;
    virtual ~INotification() = default;
};

struct SMSNotification : INotification {
    void send(const std::string& message) override {
        std::cout << "Sending SMS notification " << message << std::endl;
    }
};

class NotificationDecorator : public INotification {
protected:
    std::unique_ptr<INotification> baseNotification;

public:
    explicit NotificationDecorator(std::unique_ptr<INotification> notification)
        : baseNotification(std::move(notification)) {}
};

class FormatNotification : public NotificationDecorator {
public:
    explicit FormatNotification(std::unique_ptr<INotification> notification)
        : NotificationDecorator(std::move(notification)) {}

    void send(const std::string& message) override {
        std::cout << "Notification is formatted" << std::endl;
        baseNotification->send(message);
    }
};

class AnimationNotification : public NotificationDecorator {
public:
    explicit AnimationNotification(std::unique_ptr<INotification> notification)
        : NotificationDecorator(std::move(notification)) {}

    void send(const std::string& message) override {
        std::cout << "AnimationNotification is added" << std::endl;
        baseNotification->send(message);
    }
};

int main() {
    std::unique_ptr<INotification> notification = std::make_unique<SMSNotification>();
    notification->send("pradeeep");
    return 0;
}
