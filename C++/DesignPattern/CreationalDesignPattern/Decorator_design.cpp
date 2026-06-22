#include <iostream>
#include <memory>
#include <string>
using namespace std;

// Product interface
struct INotification {
    virtual void send(const string& message) = 0;
    virtual ~INotification() = default;
};

struct SMSNotification : INotification {
    void send(const string& message) override {
        cout << "Sending SMS notification " << message << "\n";
    }
};

// Base decorator
class NotificationDecorator : public INotification {
protected:
    shared_ptr<INotification> baseNotification;
public:
    explicit NotificationDecorator(shared_ptr<INotification> notification)
        : baseNotification(move(notification)) {}
};

struct FormatNotification : NotificationDecorator {
    explicit FormatNotification(shared_ptr<INotification> notification)
        : NotificationDecorator(move(notification)) {}

    void send(const string& message) override {
        cout << "Notification is formatted\n";
        baseNotification->send(message);
    }
};

struct AnimationNotification : NotificationDecorator {
    explicit AnimationNotification(shared_ptr<INotification> notification)
        : NotificationDecorator(move(notification)) {}

    void send(const string& message) override {
        cout << "AnimationNotification is added\n";
        baseNotification->send(message);
    }
};

int main() {
    shared_ptr<INotification> notification = make_shared<SMSNotification>();
    notification->send("pradeeep");
    return 0;
}
