#include <iostream>
#include <memory>
#include <string>
#include <sstream>
using namespace std;

// Chain of Responsibility Design Pattern - Log Handling Example

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

int severity(LogLevel level) {
    return static_cast<int>(level);
}

string levelName(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
    }
    return "UNKNOWN";
}

// Handlers
// Chain
// Request
struct Log {
    string message;
    LogLevel level;

    Log(string message, LogLevel level) : message(move(message)), level(level) {}

    LogLevel getLogLevel() const { return level; }

    string toString() const {
        ostringstream oss;
        oss << "[" << levelName(level) << "] " << message;
        return oss.str();
    }
};

class Handler {
protected:
    shared_ptr<Handler> nextHandler;

public:
    explicit Handler(shared_ptr<Handler> nextHandler) : nextHandler(move(nextHandler)) {}
    virtual ~Handler() = default;

    void callNextHandler(const Log& log) {
        if (nextHandler) {
            nextHandler->handle(log);
        }
    }

    virtual bool canHandle(const Log& log) = 0;
    virtual void handle(const Log& log) = 0;
};

class Level1 : public Handler {
public:
    explicit Level1(shared_ptr<Handler> nextHandler) : Handler(move(nextHandler)) {}

    bool canHandle(const Log& log) override {
        return severity(log.getLogLevel()) <= severity(LogLevel::INFO);
    }

    void handle(const Log& log) override {
        if (canHandle(log)) {
            cout << "Reqeust is handled by Level1\n";
            cout << "Logging the log" << log.toString() << "\n";
        } else {
            callNextHandler(log);
        }
    }
};

class Level2 : public Handler {
public:
    explicit Level2(shared_ptr<Handler> nextHandler) : Handler(move(nextHandler)) {}

    bool canHandle(const Log& log) override {
        return severity(log.getLogLevel()) == severity(LogLevel::WARN);
    }

    void handle(const Log& log) override {
        if (canHandle(log)) {
            cout << "Reqeust is handled by Level1\n";
            cout << "Logging the log" << log.toString() << "\n";
        } else {
            callNextHandler(log);
        }
    }
};

class Level3 : public Handler {
public:
    explicit Level3(shared_ptr<Handler> nextHandler) : Handler(move(nextHandler)) {}

    bool canHandle(const Log& log) override {
        return severity(log.getLogLevel()) >= severity(LogLevel::ERROR);
    }

    void handle(const Log& log) override {
        if (canHandle(log)) {
            cout << "Reqeust is handled by Level1\n";
            cout << "Logging the log" << log.toString() << "\n";
        } else {
            callNextHandler(log);
        }
    }
};

int main() {
    auto level3 = make_shared<Level3>(nullptr);
    auto level2 = make_shared<Level2>(level3);
    auto level1 = make_shared<Level1>(level2);

    level1->handle(Log("log this", LogLevel::DEBUG));
    return 0;
}
