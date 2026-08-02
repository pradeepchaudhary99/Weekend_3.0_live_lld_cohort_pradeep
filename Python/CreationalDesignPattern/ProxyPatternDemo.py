# class ProxyPatternDemo:
#
#     # Subject
#     class Database(ABC):
#         @abstractmethod
#         def fetch_data(self):
#             pass
#
#     # Real Subject
#     class RealDatabase(Database):
#         def fetch_data(self):
#             print("Fetching confidential employee salary data...")
#
#     # Proxy
#     class DatabaseProxy(Database):
#         def __init__(self, role):
#             self.role = role
#             self.database = None
#
#         def fetch_data(self):
#             if self.role.upper() != "ADMIN":
#                 print("Access Denied! Only ADMIN can access the database.")
#                 return
#
#             # Lazy Initialization
#             if self.database is None:
#                 print("Creating RealDatabase object...")
#                 self.database = ProxyPatternDemo.RealDatabase()
#
#             self.database.fetch_data()
#
#
# def main():
#     user = ProxyPatternDemo.DatabaseProxy("USER")
#     user.fetch_data()
#
#     print("--------------------------")
#
#     admin = ProxyPatternDemo.DatabaseProxy("ADMIN")
#     admin.fetch_data()
#
#     print("--------------------------")
#
#     # Same proxy reused (Real object already created)
#     admin.fetch_data()
