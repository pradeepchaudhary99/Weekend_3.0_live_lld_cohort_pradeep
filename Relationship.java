



interface INotification{
    void sendNotification(String message);
}

class SMSNotification implements INotification{

}

class EmailNotification implements INotification{
    
}

class WhatsappNotification implements INotification{
    
}

class SlackNotification implements INotification{
    
}


class NotificationClient{
    INofication notification;
    public NotificationClient(){
        notification = new SMSNotification();
    }

    public setNotification(Notification notification){
        this.notification = notification;
    }

    public void sendNotification(String message){
        notification.sendNotification(message);
    }
}