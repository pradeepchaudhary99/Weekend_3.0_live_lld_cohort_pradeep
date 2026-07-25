package CreationalDesignPattern;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.locks.ReentrantLock;

class Logger{
    static Logger instance = null;
    ReentrantLock reentrantLock = new ReentrantLock();
    List<String> logs;
    private Logger(){
        System.out.println("object created");
        logs = new ArrayList<>();
    }    
    public void appendLogs(String log){
        logs.add(log);
    }

    public static Logger getInstance_normal_thread_unsafe(){
        if(instance == null){
            instance = new Logger();
        }
        return instance;
    }

    public synchronized static Logger getInstance_synchronized(){
        if(instance == null){
            instance = new Logger();
        }
        return instance;
    }

    public  static Logger getInstance(){

        if(instance == null){
            synchronized(Logger.class){
                if(instance == null){
                    instance = new Logger();
                }
            }
        }

        return instance;
    }


}

public class SingletonDesignPattern {
    public static void main(String[] args) {
        
    //    Logger logger1 = Logger.getInstance();
    //    Logger logger2 = Logger.getInstance();
    //    Logger logger3 = Logger.getInstance();
       
       Runnable task1 = ()->{
        Logger.getInstance();
       };

    //    Thread thread1 = new Thread(task1);
    //    Thread thread2 = new Thread(task1);
    //    thread1.start();
    //    thread2.start();

    for(int i = 0; i < 2; i++){
        new Thread(task1).start();
    }


    

    } 

}
