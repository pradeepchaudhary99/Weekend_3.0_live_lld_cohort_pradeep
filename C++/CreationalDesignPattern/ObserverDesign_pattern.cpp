#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct Follower {
    virtual void notify(const std::string& message) = 0;
    virtual ~Follower() = default;
};

struct Subject {
    virtual void addObserver(std::shared_ptr<Follower> observer) = 0;
    virtual void removeObserver(std::shared_ptr<Follower> observer) = 0;
    virtual ~Subject() = default;
};

class WhatsAppBroadCast : public Subject {
    std::vector<std::shared_ptr<Follower>> followers;

public:
    void addObserver(std::shared_ptr<Follower> observer) override {
        followers.push_back(observer);
    }

    void removeObserver(std::shared_ptr<Follower> observer) override {
        followers.erase(std::remove(followers.begin(), followers.end(), observer), followers.end());
    }

    void addFollower(std::shared_ptr<Follower> follower) { addObserver(follower); }

    void removeFollower(std::shared_ptr<Follower> follower) { removeObserver(follower); }

    void sendMessage(const std::string& message) {
        for (auto& follower : followers) {
            follower->notify(message);
        }
    }
};

class Prateek : public Follower {
public:
    void notify(const std::string& message) override {
        std::cout << " Prateek Received Message: " << message << std::endl;
    }
};

class Abhinav : public Follower {
public:
    void notify(const std::string& message) override {
        std::cout << "Abhinav Received Message: " << message << std::endl;
    }
};

int main() {
    WhatsAppBroadCast whatsAppBroadCast;
    whatsAppBroadCast.addFollower(std::make_shared<Prateek>());
    whatsAppBroadCast.addFollower(std::make_shared<Abhinav>());

    whatsAppBroadCast.sendMessage("pradeep is teaching Observer pattern");

    return 0;
}
