#include <iostream>
#include <memory>

struct IPaymentProcessor {
    virtual void pay() = 0;
    virtual ~IPaymentProcessor() = default;
};

struct InHousePaymentProcessor : IPaymentProcessor {
    void pay() override {
        std::cout << "In House Application is processing the payment" << std::endl;
        // 100000 lines of code
    }
};

class Razorpay {
public:
    void makePayment() {
        std::cout << "Razorpay Payment 10203013203103" << std::endl;
    }
};

struct RazorPayAdapter : IPaymentProcessor {
    Razorpay razorpay;

    void pay() override {
        razorpay.makePayment();
    }
};

class Application {
    std::unique_ptr<IPaymentProcessor> paymentProcessor;

public:
    explicit Application(std::unique_ptr<IPaymentProcessor> processor)
        : paymentProcessor(std::move(processor)) {}

    void processPayment() {
        paymentProcessor->pay();
    }
};

int main() {
    Application app(std::make_unique<RazorPayAdapter>());  // factory work
    app.processPayment();
    return 0;
}
