
interface IPaymentProcessor{
    void pay();
}

class InHousePaymentProcessor implements IPaymentProcessor{
    @Override
    public void pay() {
        System.out.println("In House Application is processing the payment");
        /*100000 lines of codes */
    }
}

class Razorpay{
    public void make_payment(){
        System.out.println("Razorpay Payment 10203013203103");
    }
}

class RazorPayAdapter implements IPaymentProcessor{
    Razorpay razorpay;
    public RazorPayAdapter(){
        razorpay = new Razorpay();
    }

    @Override
    public void pay() {
        razorpay.make_payment();
    } 
}

class Application{
    IPaymentProcessor paymentProcessor;
    public Application(IPaymentProcessor processor){
        this.paymentProcessor = processor;
    }
    public void processPayment(){
        paymentProcessor.pay();
    }
}

public class AdapterDesignPattern {
    public static void main(String[] args) {
        Application app = new Application(new RazorPayAdapter()); //factory work
        app.processPayment();
    }
}
