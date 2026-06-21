// Factory Design Pattern
#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>
using namespace std;

class INotification {
public:
    virtual void send() = 0;
    virtual ~INotification() {}
};

class SMSNotification : public INotification {
public:
    void send() override { cout << "Sending SMS notification\n"; }
};

class WhatsAppNotification : public INotification {
public:
    void send() override { cout << "Sending WhatsApp notification\n"; }
};

class SlackNotification : public INotification {
public:
    void send() override { cout << "Sending Slack notification\n"; }
};

class NotificationFactory {
    static unordered_map<string, shared_ptr<INotification>> registry;
public:
    static shared_ptr<INotification> getInstance(const string& type) {
        if (type == "SMS" && !registry.count(type))
            registry[type] = make_shared<SMSNotification>();
        else if (type == "whatsapp" && !registry.count(type))
            registry[type] = make_shared<WhatsAppNotification>();
        else if (type == "Slack" && !registry.count(type))
            registry[type] = make_shared<SlackNotification>();

        return registry.count(type) ? registry[type] : nullptr;
    }
};

unordered_map<string, shared_ptr<INotification>> NotificationFactory::registry;

class NotificationService {
public:
    void sendNotification(const string& type, const string& message) {
        auto notification = NotificationFactory::getInstance(type);
        if (notification) notification->send();
    }
};

int main() {
    NotificationService service;
    service.sendNotification("SMS", "Hello!");
    service.sendNotification("whatsapp", "Hello!");
    service.sendNotification("Slack", "Hello!");
    return 0;
}
