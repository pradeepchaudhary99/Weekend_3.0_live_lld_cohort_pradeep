// Builder Design Pattern
#include <iostream>
#include <string>
using namespace std;

class Student {
    string name;        // mandatory
    int rollNumber;
    int age;
    int grade;
    string phoneNumber;

    Student() {}

public:
    void print() const {
        cout << "Student[name=" << name
             << ", roll=" << rollNumber
             << ", age=" << age
             << ", grade=" << grade
             << ", phone=" << phoneNumber << "]\n";
    }

    class StudentBuilder {
        friend class Student;
        string name;
        int rollNumber = 0;
        int age        = 0;
        int grade      = 0;
        string phoneNumber;

    public:
        StudentBuilder(const string& name) : name(name) {}

        StudentBuilder& setRollNumber(int r) { rollNumber = r; return *this; }
        StudentBuilder& setAge(int a)        { age = a;        return *this; }
        StudentBuilder& setGrade(int g)      { grade = g;      return *this; }
        StudentBuilder& setPhoneNumber(const string& p) { phoneNumber = p; return *this; }

        Student build() {
            Student s;
            s.name        = name;
            s.rollNumber  = rollNumber;
            s.age         = age;
            s.grade       = grade;
            s.phoneNumber = phoneNumber;
            return s;
        }
    };
};

int main() {
    Student student = Student::StudentBuilder("pradeep")
                        .setGrade(2)
                        .setPhoneNumber("9999999999")
                        .build();
    student.print();
    return 0;
}
