// Factory Method Design Pattern
#include <iostream>
#include <memory>
using namespace std;

// Product interface
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

// Factory interface
class INotificationFactory {
public:
    virtual shared_ptr<INotification> getNotification() = 0;
    virtual ~INotificationFactory() {}
};

class SMSNotificationFactory : public INotificationFactory {
public:
    shared_ptr<INotification> getNotification() override {
        return make_shared<SMSNotification>();
    }
};

class WhatsAppNotificationFactory : public INotificationFactory {
public:
    shared_ptr<INotification> getNotification() override {
        return make_shared<WhatsAppNotification>();
    }
};

class SlackNotificationFactory : public INotificationFactory {
public:
    shared_ptr<INotification> getNotification() override {
        return make_shared<SlackNotification>();
    }
};

class NotificationService {
public:
    void sendNotification(INotificationFactory* factory) {
        auto notification = factory->getNotification();
        notification->send();
    }
};

int main() {
    NotificationService service;

    SMSNotificationFactory smsFactory;
    service.sendNotification(&smsFactory);

    WhatsAppNotificationFactory waFactory;
    service.sendNotification(&waFactory);

    SlackNotificationFactory slackFactory;
    service.sendNotification(&slackFactory);

    return 0;
}
