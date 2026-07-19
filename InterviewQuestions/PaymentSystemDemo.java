

/*

Functional Requirements:
    User should be able to initiate a payment 
    Support multiple payment methods [UPI, CARD, NetBanking] :[Strategy]
    ** Prevent duplicate payments {Idempotency}
    Payment Status : PENDING, SUCCESS, FAILED 
    Retry Failed Payments : Decorator Design Patterns 
    Store payment History 


Non-Functional Requirements:
Thread-safe 
Extensible
No Duplicate Charges
Async Processing    : ThreadPool we can achieve this 



Entities:
PaymentStatus 
Payment
PaymentMethod 
PaymentService
IdempotencyManager/ IdempotencyRepository


*/


// RequestPayment{
//     Payment 
//     Idempotency;
// }

import java.time.LocalDateTime;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ExecutorService;

class Payment{
    String paymentId;
    String orderId;
    double amount;
    PaymentStatus status;
    LocalDateTime time;
}

enum PaymentStatus{
    PENDING,
    SUCCESS,
    FAILED
}


interface PaymentMethod{
    boolean validate();
}

interface PaymentGateway{
    boolean processPayment(Payment payment);
}

class IdempotencyRepository{
    Map<String, Payment> repository;    // Key: IdempotencyKey, Payment
}

class PaymentRepository{
    Map<String, Payment> paymentRepository; // Key: PaymentId, Payment
}



class PaymentService{
    IdempotencyRepository idempotencyRepository;
    PaymentRepository paymentRepository;
    PaymentMethod paymentMethod;
    PaymentGateway paymentGateway;

    ExecutorService executor;   //

    public Response createPayment(Request request){

        // Step 1: validate Request for duplicacy
        if(idempotencyRepository.containsKey(request.idempotencyKey)){
            return response[Duplicate Request, Payment Status];
        }else{
            // create the entry in the Idempotency Repository and store to avoid future duplicate entries
        }

        Payment payment = new Payment(Take params from the request and fill this payment creation);
        String paymentId = UUID.randomUUID();   // Distributed Key generator
        paymentRepository.put(paymentId, payment);
        
        if(paymentMethod.validate(payment)){
            executor.submit(()->{
                paymentGateway.processPayment(payment);
            });
        }else{
            //failed 
            // Undo 
        }
    }
}

public class PaymentSystemDemo {
    
}

