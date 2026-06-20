package DesignPattern.CreationalDesignPattern;

import java.rmi.NotBoundException;
import java.util.HashMap;

import javax.print.attribute.HashAttributeSet;

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


//Problem
// class NotificationService{

//     void sendNotification(String type, String message){
//         INotification notification = null;
//         if(type.equals("SMS")){
//             notification = new SMSNotification();
//         }else if(type.equals("whatsapp")){
//             notification = new WhatsAppNotification();
//         }
//     }
// }



class NotificationService{
    void sendNotification(String type, String message){
        INotification notification = NotificationFactory.getInstance(type);
        //validation
        // 10000
        notification.send();
    }
}

class NotificationFactory{
    static HashMap<String, INotification> registry = new HashMap<>(); //
    static INotification getInstance(String type){
        if(type.equals("SMS")){    
            registry.putIfAbsent(type, new SMSNotification());
        }else if(type.equals("whatsapp")){    
            registry.putIfAbsent(type, new WhatsAppNotification());
        }
        else if(type.equals("Slack")){
            
        }
        return registry.get(type);
    }
}



public class FactoryDesignPattern {
    
}
