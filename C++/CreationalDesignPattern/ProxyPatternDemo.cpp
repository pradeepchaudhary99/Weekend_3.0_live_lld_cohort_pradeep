// struct Database {
//     virtual void fetchData() = 0;
//     virtual ~Database() = default;
// };
//
// // Real Subject
// struct RealDatabase : Database {
//     void fetchData() override {
//         std::cout << "Fetching confidential employee salary data..." << std::endl;
//     }
// };
//
// // Proxy
// class DatabaseProxy : public Database {
//     std::string role;
//     std::unique_ptr<RealDatabase> database;
//
// public:
//     explicit DatabaseProxy(std::string role) : role(std::move(role)) {}
//
//     void fetchData() override {
//         if (role != "ADMIN") {
//             std::cout << "Access Denied! Only ADMIN can access the database." << std::endl;
//             return;
//         }
//
//         // Lazy Initialization
//         if (!database) {
//             std::cout << "Creating RealDatabase object..." << std::endl;
//             database = std::make_unique<RealDatabase>();
//         }
//
//         database->fetchData();
//     }
// };
//
// int main() {
//     std::unique_ptr<Database> user = std::make_unique<DatabaseProxy>("USER");
//     user->fetchData();
//
//     std::cout << "--------------------------" << std::endl;
//
//     std::unique_ptr<Database> admin = std::make_unique<DatabaseProxy>("ADMIN");
//     admin->fetchData();
//
//     std::cout << "--------------------------" << std::endl;
//
//     // Same proxy reused (Real object already created)
//     admin->fetchData();
// }
