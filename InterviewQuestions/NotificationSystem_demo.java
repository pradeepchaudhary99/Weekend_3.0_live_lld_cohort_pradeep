
/*
FunctionalRequirements:

Users should be able to send Notification
System should be able to support multiple types of  channels
    --> Email
    --> SMS
    --> Push
    --> Whatsapp 
    --> Slack
Notification System should care about user preference
Notificaion sending should be asynchronous 
Easy to add new notification types/channels


Entities:

User --> Client ---> main --> class Clients 
            // SendNotification

Notification 
NotificationService 
NotificationChannel 
UserPreferencesRepository
NotificationChannelFactory 




*/

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

class Notification{
    String recipient;
    String title;
    String message;
}

interface NotificationChannel{
    void sendNotification(Notification notification);
}




abstract class  NotificationDecorator implements NotificationChannel{
     protected NotificationChannel notificationChannel;
     public NotificationDecorator(NotificationChannel notification){
        this.notificationChannel = notification;
     }
}

class RateLimiterDecorator extends NotificationDecorator{

    public RateLimiterDecorator(NotificationChannel notificationChannel){
        super(notificationChannel);
    }
    @Override
    public void sendNotification(Notification notification) {
        // 1000 lines of rate limiting logic will be here 
        notificationChannel.sendNotification(notification);   
    }
}


class SMSNotification implements NotificationChannel{

    @Override
    public void sendNotification(Notification notification) {

    }
    
}

class PushNotification implements NotificationChannel{

    @Override
    public void sendNotification(Notification notification) {

    }
}

class WhatsappNotification implements NotificationChannel{

    @Override
    public void sendNotification(Notification notification) {

    }
    
}

class EmailNotification implements NotificationChannel{

    @Override
    public void sendNotification(Notification notification) {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'sendNotification'");
    }
    
}

enum NotificationType{
    SMS, PUSH, EMAIL, WHATSAPP
}

class NotificationFactory{
  Map<NotificationType, NotificationChannel> registry;
  
  public NotificationFactory(){
    registry = new HashMap<>();
    registry.put(NotificationType.EMAIL, new EmailNotification());
    registry.put(NotificationType.SMS, new EmailNotification());
    registry.put(NotificationType.WHATSAPP, new EmailNotification());
    registry.put(NotificationType.SMS, new EmailNotification());
  }
  
  NotificationChannel getInstance(NotificationType type){
    return registry.get(type);
  }
}


class UserPreferencesRepository{
    Map<String, Set<NotificationChannel>> userPreferences;

    Set<NotificationChannel> getUserPreference(String userId){

    }
}
//Orchasttor, Facade

class NotificationService{
    ExecutorService executor;
    NotificationFactory factory; 
    UserPreferencesRepository userPreferencesRepository;

    public NotificationService(){
        executor = Executors.newFixedThreadPool(4);
        factory = new NotificationFactory();
    }


    public void send(Notification notification){
        Set<NotificationChannel> preferedChannels = userPreferencesRepository.getUserPreference(notification.recipient);
        for(NotificationChannel channel : preferedChannels){
            // SMS, Whatsapp, GMAIL
            //Asycn
            executor.submit(()->{
                channel.sendNotification(notification);
            });
            // channel.sendNotification(notification); //sync
        }
    }
}


//Applying Decorator Design Pattern:
// Apply Decoraton design pattern to enforce Retry Logic, Rate limiting, Formatting

class NotificationClient{

    void SendNotification(NotificationService notificationService, Notification notification){

    }
}


public class NotificationSystem_demo {
    
}
