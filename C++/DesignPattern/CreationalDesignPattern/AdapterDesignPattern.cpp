#include <iostream>
#include <memory>
using namespace std;

struct IPaymentProcessor {
    virtual void pay() = 0;
    virtual ~IPaymentProcessor() = default;
};

struct InHousePaymentProcessor : IPaymentProcessor {
    void pay() override {
        cout << "In House Application is processing the payment\n";
        // 100000 lines of codes
    }
};

struct Razorpay {
    void make_payment() {
        cout << "Razorpay Payment 10203013203103\n";
    }
};

struct RazorPayAdapter : IPaymentProcessor {
    RazorPayAdapter() : razorpay(make_unique<Razorpay>()) {}

    void pay() override {
        razorpay->make_payment();
    }
private:
    unique_ptr<Razorpay> razorpay;
};

class Application {
    unique_ptr<IPaymentProcessor> paymentProcessor;
public:
    explicit Application(unique_ptr<IPaymentProcessor> processor)
        : paymentProcessor(move(processor)) {}

    void processPayment() {
        paymentProcessor->pay();
    }
};

int main() {
    Application app(make_unique<RazorPayAdapter>());
    app.processPayment();
    return 0;
}
