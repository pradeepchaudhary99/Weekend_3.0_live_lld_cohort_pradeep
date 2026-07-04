// #include <iostream>
// #include <memory>
// using namespace std;
//
// // Subject
// struct Database {
//     virtual void fetchData() = 0;
//     virtual ~Database() = default;
// };
//
// // Real Subject
// struct RealDatabase : Database {
//     void fetchData() override {
//         cout << "Fetching confidential employee salary data...\n";
//     }
// };
//
// // Proxy
// class DatabaseProxy : public Database {
//     string role;
//     unique_ptr<RealDatabase> database;
//
// public:
//     explicit DatabaseProxy(const string& role) : role(role) {}
//
//     void fetchData() override {
//         if (role != "ADMIN") {
//             cout << "Access Denied! Only ADMIN can access the database.\n";
//             return;
//         }
//
//         // Lazy Initialization
//         if (!database) {
//             cout << "Creating RealDatabase object...\n";
//             database = make_unique<RealDatabase>();
//         }
//
//         database->fetchData();
//     }
// };
//
// int main() {
//     unique_ptr<Database> user = make_unique<DatabaseProxy>("USER");
//     user->fetchData();
//
//     cout << "--------------------------\n";
//
//     unique_ptr<Database> admin = make_unique<DatabaseProxy>("ADMIN");
//     admin->fetchData();
//
//     cout << "--------------------------\n";
//
//     // Same proxy reused (Real object already created)
//     admin->fetchData();
//
//     return 0;
// }
