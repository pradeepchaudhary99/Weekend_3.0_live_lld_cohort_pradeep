#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

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

// Problem
// class NotificationService {
//     void sendNotification(std::string type, std::string message) {
//         std::shared_ptr<INotification> notification = nullptr;
//         if (type == "SMS") {
//             notification = std::make_shared<SMSNotification>();
//         } else if (type == "whatsapp") {
//             notification = std::make_shared<WhatsAppNotification>();
//         }
//     }
// };

class NotificationFactory {
public:
    static std::unordered_map<std::string, std::shared_ptr<INotification>> registry;

    static std::shared_ptr<INotification> getInstance(const std::string& type) {
        if (type == "SMS") {
            registry.emplace(type, std::make_shared<SMSNotification>());
        } else if (type == "whatsapp") {
            registry.emplace(type, std::make_shared<WhatsAppNotification>());
        } else if (type == "Slack") {
        }
        auto it = registry.find(type);
        return it == registry.end() ? nullptr : it->second;
    }
};

std::unordered_map<std::string, std::shared_ptr<INotification>> NotificationFactory::registry;

class NotificationService {
public:
    void sendNotification(const std::string& type, const std::string& message) {
        auto notification = NotificationFactory::getInstance(type);
        // validation
        // 10000
        notification->send();
    }
};

class FactoryDesignPattern {};
