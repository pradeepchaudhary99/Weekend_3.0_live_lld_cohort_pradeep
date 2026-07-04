#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

struct Subject {
    virtual void addObserver(struct Observer* observer) = 0;
    virtual void removeObserver(struct Observer* observer) = 0;
    virtual ~Subject() = default;
};

struct Follower {
    virtual void notify(const string& message) = 0;
    virtual ~Follower() = default;
};

class WhatsAppBroadCast {
    vector<shared_ptr<Follower>> followers;

public:
    void addFollower(const shared_ptr<Follower>& follower) {
        followers.push_back(follower);
    }

    void removeFollower(const shared_ptr<Follower>& follower) {
        followers.erase(remove(followers.begin(), followers.end(), follower), followers.end());
    }

    void sendMessage(const string& message) {
        for (auto& follower : followers) {
            follower->notify(message);
        }
    }
};

struct Prateek : Follower {
    void notify(const string& message) override {
        cout << " Prateek Received Message: " << message << "\n";
    }
};

struct Abhinav : Follower {
    void notify(const string& message) override {
        cout << "Abhinav Received Message: " << message << "\n";
    }
};

int main() {
    WhatsAppBroadCast whatsAppBroadCast;
    whatsAppBroadCast.addFollower(make_shared<Prateek>());
    whatsAppBroadCast.addFollower(make_shared<Abhinav>());

    whatsAppBroadCast.sendMessage("pradeep is teaching Observer pattern");
    return 0;
}
