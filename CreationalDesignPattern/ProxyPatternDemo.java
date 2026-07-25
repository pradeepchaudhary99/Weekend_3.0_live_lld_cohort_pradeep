package CreationalDesignPattern;
// public class ProxyPatternDemo {

//     // Subject
//     interface Database {
//         void fetchData();
//     }

//     // Real Subject
//     static class RealDatabase implements Database {

//         @Override
//         public void fetchData() {
//             System.out.println("Fetching confidential employee salary data...");
//         }
//     }

//     // Proxy
//     static class DatabaseProxy implements Database {

//         private String role;
//         private RealDatabase database;

//         public DatabaseProxy(String role) {
//             this.role = role;
//         }

//         @Override
//         public void fetchData() {

//             if (!role.equalsIgnoreCase("ADMIN")) {
//                 System.out.println("Access Denied! Only ADMIN can access the database.");
//                 return;
//             }

//             // Lazy Initialization
//             if (database == null) {
//                 System.out.println("Creating RealDatabase object...");
//                 database = new RealDatabase();
//             }

//             database.fetchData();
//         }
//     }

//     public static void main(String[] args) {

//         Database user = new DatabaseProxy("USER");
//         user.fetchData();

//         System.out.println("--------------------------");

//         Database admin = new DatabaseProxy("ADMIN");
//         admin.fetchData();

//         System.out.println("--------------------------");

//         // Same proxy reused (Real object already created)
//         admin.fetchData();
//     }
// }