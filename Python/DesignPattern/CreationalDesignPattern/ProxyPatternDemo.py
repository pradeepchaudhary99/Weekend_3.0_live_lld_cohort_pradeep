# from abc import ABC, abstractmethod
#
#
# class Database(ABC):
#     @abstractmethod
#     def fetch_data(self): pass
#
#
# class RealDatabase(Database):
#     def fetch_data(self):
#         print("Fetching confidential employee salary data...")
#
#
# class DatabaseProxy(Database):
#     def __init__(self, role: str):
#         self.role = role
#         self.database = None
#
#     def fetch_data(self):
#         if self.role.upper() != "ADMIN":
#             print("Access Denied! Only ADMIN can access the database.")
#             return
#
#         # Lazy Initialization
#         if self.database is None:
#             print("Creating RealDatabase object...")
#             self.database = RealDatabase()
#
#         self.database.fetch_data()
#
#
# if __name__ == "__main__":
#     user = DatabaseProxy("USER")
#     user.fetch_data()
#
#     print("--------------------------")
#
#     admin = DatabaseProxy("ADMIN")
#     admin.fetch_data()
#
#     print("--------------------------")
#
#     # Same proxy reused (Real object already created)
#     admin.fetch_data()
