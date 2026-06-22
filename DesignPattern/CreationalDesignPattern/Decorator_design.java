package DesignPattern.CreationalDesignPattern;

//Product 
interface INotification{
    void send(String message);
}

class SMSNotification implements INotification{
    @Override
    public void send(String message) {
        System.out.println("Sending SMS notification " + message);

    }
}

abstract class NotificationDecorator implements INotification{
    INotification baseNotification;
    public NotificationDecorator(INotification notification){
        this.baseNotification = notification;
    }
}

class FormatNotification extends NotificationDecorator{

    public FormatNotification(INotification notification){
        super(notification);
    }
    @Override
    public void send(String message) {
        System.out.println("Notification is formatted");
        baseNotification.send(message);
    }   
}

class AnimationNotification extends NotificationDecorator{

    public AnimationNotification(INotification notification){
        super(notification);
    }
    @Override
    public void send(String message) {
        System.out.println("AnimationNotification is added");
        baseNotification.send(message);
    }   
}



public class Decorator_design {
    public static void main(String[] args) {
        INotification notification = new SMSNotification();
        notification.send("pradeeep");
    }
}
