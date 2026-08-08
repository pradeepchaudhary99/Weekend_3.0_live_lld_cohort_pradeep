

// ThreadPool 
//     ===> Workers 
//     ===> BlockingQueue 
//     ===> Task 

// // Producer ----> ading task to the queue 
// // Consumer ----> Picking the task from  the queue and processing it..

import java.util.Queue;

import javax.management.RuntimeErrorException;

interface Task{
    void execute();
}


class RunnableTask implements Task{
    Runnable task;
    public void setTask(Runnable task){
        this.task = task;
    }
    @Override
    public void execute() {
        task.run();
    }
}

class TaskQueue{
    Queue<Task> queue;
    int capacity;
    synchronized void enqueue(Task task){
        while(queue.size() == capacity){
           try{
             wait();
            } catch(InterruptedException e){
                throw new RuntimeErrorException(e);
            }
        }
        queue.offer(task);
        notifyAll();    // producer asking consumer to wake up
    }

    synchronized Task dequeue(){
        while(queue.isEmpty()){
            wait();
        }
        Task task = queue.poll();
        notifyAll();    // consumer asking producer to wake up
        return task;
    }
}


class Worker implements Runnable{
    TaskQueue taskQueue; 
    boolean running = true;
    
    public void shutdown(){
        running = false;
    }

    @Override
    public void run() {
        while(running){
            Task task = taskQueue.dequeue();
            if(task != null){
                task.execute();
            }        
        }
    }
}


class ThreadPool{
    TaskQueue taskQueue;
    List<Thread> workers;
    boolean shutdown = false;
    int numberOfThreads;
    /*
        List<Thread> activeWorker;
        List<Thread> standbyThread;

    */
    void createWorkers(){
        for(int i = 0; i < numberOfThreads; i++){
            Thread thread = new Thread(new Worker());
            workers.add(thread);
        }
    }

    void submit(Task task){
        taskQueue.enqueue(task);
    }

    void shutdown(){
        shutdown = true;
    }
}



public class ThreadPool_demo {
    
}
