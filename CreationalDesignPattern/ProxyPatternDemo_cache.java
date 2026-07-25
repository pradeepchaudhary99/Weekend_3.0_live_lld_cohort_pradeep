package CreationalDesignPattern;
import java.util.HashMap;




/*

fun non 
entitieis

relationsji

class Notification
    - priority
    - message  


*/


interface IRateLimitinStrategy{
    
}

class Notification{
    String priority;
    String message;
}

public class ProxyPatternDemo {

    // Subject
    interface Database {
        String fetchData(String query);
    }

    // Real Subject
    static class RealDatabase implements Database {

        @Override
        public String fetchData(String query) {
            System.out.println("Fetching confidential employee salary data...");
        }
    }

    // Proxy
    static class DatabaseProxy implements Database {

        private String role;
        private RealDatabase database;
        HashMap<String,String> cache;
        public DatabaseProxy(String role) {
            cache = new HashMap<>();
            this.role = role;
        }

        @Override
        public String fetchData(String query) {
            
            if(cache.containsKey(query)){
                return cache.get(query);
            }

            String value = database.fetchData(query);
            cache.put(query, value);
        }
    }

    public static void main(String[] args) {

        // Database user = new DatabaseProxy("USER");
        // user.fetchData();

        // System.out.println("--------------------------");

        // Database admin = new DatabaseProxy("ADMIN");
        // admin.fetchData();

        // System.out.println("--------------------------");

        // Same proxy reused (Real object already created)
        // admin.fetchData();

        Database user = new DatabaseProxy(null);
        user.fetchData("dsadsadasd");

    }
}