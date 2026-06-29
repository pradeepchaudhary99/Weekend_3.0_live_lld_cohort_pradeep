from abc import ABC, abstractmethod


class IPaymentProcessor(ABC):
    @abstractmethod
    def pay(self): pass


class InHousePaymentProcessor(IPaymentProcessor):
    def pay(self):
        print("In House Application is processing the payment")
        # 100000 lines of codes


class Razorpay:
    def make_payment(self):
        print("Razorpay Payment 10203013203103")


class RazorPayAdapter(IPaymentProcessor):
    def __init__(self):
        self._razorpay = Razorpay()

    def pay(self):
        self._razorpay.make_payment()


class Application:
    def __init__(self, processor: IPaymentProcessor):
        self._payment_processor = processor

    def process_payment(self):
        self._payment_processor.pay()


if __name__ == "__main__":
    app = Application(RazorPayAdapter())
    app.process_payment()
