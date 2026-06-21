// Notification system — Dependency Injection via composition
#include <iostream>
#include <string>
using namespace std;

class INotification {
public:
    virtual void sendNotification(const string& message) = 0;
    virtual ~INotification() {}
};

class SMSNotification : public INotification {
public:
    void sendNotification(const string& message) override {
        cout << "SMS: " << message << "\n";
    }
};

class EmailNotification : public INotification {
public:
    void sendNotification(const string& message) override {
        cout << "Email: " << message << "\n";
    }
};

class WhatsAppNotification : public INotification {
public:
    void sendNotification(const string& message) override {
        cout << "WhatsApp: " << message << "\n";
    }
};

class SlackNotification : public INotification {
public:
    void sendNotification(const string& message) override {
        cout << "Slack: " << message << "\n";
    }
};

class NotificationClient {
    INotification* notification;
public:
    NotificationClient() {
        notification = new SMSNotification();
    }

    void setNotification(INotification* n) {
        notification = n;
    }

    void sendNotification(const string& message) {
        notification->sendNotification(message);
    }

    ~NotificationClient() { delete notification; }
};

int main() {
    NotificationClient client;
    client.sendNotification("Hello via SMS");

    SlackNotification slack;
    client.setNotification(&slack);
    client.sendNotification("Hello via Slack");

    return 0;
}
