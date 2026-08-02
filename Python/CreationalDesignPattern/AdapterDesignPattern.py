from abc import ABC, abstractmethod


class IPaymentProcessor(ABC):
    @abstractmethod
    def pay(self):
        pass


class InHousePaymentProcessor(IPaymentProcessor):
    def pay(self):
        print("In House Application is processing the payment")
        # 100000 lines of code


class Razorpay:
    def make_payment(self):
        print("Razorpay Payment 10203013203103")


class RazorPayAdapter(IPaymentProcessor):
    def __init__(self):
        self.razorpay = Razorpay()

    def pay(self):
        self.razorpay.make_payment()


class Application:
    def __init__(self, processor: IPaymentProcessor):
        self.payment_processor = processor

    def process_payment(self):
        self.payment_processor.pay()


def main():
    app = Application(RazorPayAdapter())  # factory work
    app.process_payment()


if __name__ == "__main__":
    main()
