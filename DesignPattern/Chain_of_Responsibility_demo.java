package DesignPattern;

import java.util.logging.Level;

//Chain of Chain_of_Responsibility_demo


enum LogLevel { 
    DEBUG(0), 
    INFO(1), 
    WARN(2), 
    ERROR(3), 
    FATAL(4); 
    private final int severity; 
    LogLevel(int severity) 
    { 
      this.severity = severity; 
    } 
    public int getSeverity() 
    {
      return severity; 
    } 
}

// Handlers
// Chain
// Request 

class Log{
    String message;
    LogLevel level;

    public Log(String message, LogLevel level){
        
    }
    LogLevel getLogLevel(){
        return level;
    }
}

abstract class Handler{
    private Handler nextHandler;
    public Handler(Handler nextHandler){
        this.nextHandler = nextHandler;
    }
    void call_next_handler(Log log){
        nextHandler.handle(log);
    }
    abstract boolean canHandle(Log log);
    abstract void handle(Log log);
}

class Level1 extends Handler{
    
    public Level1(Handler nextHandler){
        super(nextHandler);
    }
    @Override
    public boolean canHandle(Log log) {
        //condition to check wether you can handle the current request/log 
        // To be completed
  
    }

    @Override
    public void handle(Log log) {
        if(canHandle(log)){
            System.out.println("Reqeust is handled by Level1");
            //Logic to handle the current request 
            System.out.println("Logging the log" + log);
        }else{
            call_next_handler(log);
        }
    }
}

class Level2 extends Handler{

    public Level2(Handler nextHandler){
        super(nextHandler);
    }
    @Override
    public boolean canHandle(Log log) {
        //condition to check wether you can handle the current request/log 
        // To be completed
        
    }

    @Override
    public void handle(Log log) {
        if(canHandle(log)){
            System.out.println("Reqeust is handled by Level1");
            //Logic to handle the current request 
            System.out.println("Logging the log" + log);
        }else{
            super.call_next_handler(log);
        }
    }
}

class Level3 extends Handler{

    public Level3(Handler nextHandler){
        super(nextHandler);
    }
    @Override
    public boolean canHandle(Log log) {
        //condition to check wether you can handle the current request/log 
        // To be completed
    }

    @Override
    public void handle(Log log) {
        if(canHandle(log)){
            System.out.println("Reqeust is handled by Level1");
            //Logic to handle the current request 
            System.out.println("Logging the log" + log);
        }else{
            call_next_handler(log);
        }
    }
}





public class Chain_of_Responsibility_demo {
    public static void main(String[] args) {
        Level3 level3 = new Level3(null);
        Level2 level2 = new Level2(level3);
        Level1 level1 = new Level1(level2);

        level1.handle(new Log("log this",LogLevel.DEBUG));

    }
}
