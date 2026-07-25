package CreationalDesignPattern;


//Product Interface
interface INotification{
    void send();
}

class SMSNotification implements INotification{

    @Override
    public void send() {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'send'");
    }

}

class WhatsAppNotification implements INotification{

    @Override
    public void send() {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'send'");
    }

}


interface INotificationFactory{
    INotification getNotification();
}

class SMSNotificationFactory implements INotificationFactory{

    @Override
    public INotification getNotification() {
        return new SMSNotification();
    }

}

class WhatsAppNotificationFactory implements INotificationFactory{

    @Override
    public INotification getNotification() {
        return new WhatsAppNotification();
    }
    
}

class Slack implements INotification{

}

class SlackNotificationFactory implements INotificationFactory{

}

class NotificationService{

    void sendNotification(INotificationFactory factory){
        INotification notification = factory.getNotification();
        notification.send();
    }
}


public class FactoryMethodDesignpattern {
    public static void main(String[] args) {
        NotificationService service = new NotificationService();
        service.sendNotification(new SMSNotificationFactory());
    }
}
