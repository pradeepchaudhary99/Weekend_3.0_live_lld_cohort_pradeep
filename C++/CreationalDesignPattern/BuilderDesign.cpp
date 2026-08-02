#include <string>

class Student {
public:
    class StudentBuilder {
    public:
        std::string name;  // mandatory
        // optional fields
        int rollNumber = 11;
        long age = 12;
        int grade = 12;
        std::string phoneNumber = "dadsadasd";

        explicit StudentBuilder(const std::string& name) : name(name) {}

        StudentBuilder& setRollNumber(int rollNumber) {
            this->rollNumber = rollNumber;
            return *this;
        }

        StudentBuilder& setAge(long age) {
            this->age = age;
            return *this;
        }

        StudentBuilder& setGrade(int grade) {
            this->grade = grade;
            return *this;
        }

        StudentBuilder& setPhoneNumber(const std::string& phoneNumber) {
            this->phoneNumber = phoneNumber;
            return *this;
        }

        Student build() {
            return Student(*this);
        }
    };

    explicit Student(const StudentBuilder& builder)
        : name(builder.name),
          rollNumber(builder.rollNumber),
          age(builder.age),
          grade(builder.grade),
          phoneNumber(builder.phoneNumber) {}

private:
    std::string name;  // mandatory
    int rollNumber;
    long age;
    int grade;
    std::string phoneNumber;
};

int main() {
    Student student = Student::StudentBuilder("pradeep")
                           .setGrade(2)
                           .setPhoneNumber("padsad")
                           .build();

    return 0;
}
