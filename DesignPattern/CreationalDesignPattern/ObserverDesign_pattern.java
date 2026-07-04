import java.util.ArrayList;
import java.util.List;

interface Subject{
    void addObserver(Observer observer);
    void removeObserver(Observer observer);
}

class WhatsAppBroadCast {

    List<Follower> followers = new ArrayList<>();

    void addFollower(Follower follower){
        followers.add(follower);
    }
    void removeFollower(Follower follower){
        followers.remove(follower);
    }

    void sendMessage(String message){
        for(Follower follower: followers){
            follower.notify(message);
        }
    }

}


interface Follower{
    void notify(String message);
}


class Prateek implements Follower{
    @Override
    public void notify(String message) {
        System.out.println(" Prateek Received Message: "+message);
    }
}

class Abhinav implements Follower{
    @Override
    public void notify(String message) {
        System.out.println("Abhinav Received Message: "+message);
    }
}


public class ObserverDesign_pattern {
    public static void main(String[] args) {
        WhatsAppBroadCast whatsAppBroadCast = new WhatsAppBroadCast();
        whatsAppBroadCast.addFollower(new Prateek());
        whatsAppBroadCast.addFollower(new Abhinav());

        whatsAppBroadCast.sendMessage("pradeep is teaching Observer pattern");
    }
}
