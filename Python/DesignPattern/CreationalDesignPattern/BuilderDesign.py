# Builder Design Pattern


class Student:
    def __init__(self, builder):
        self.name = builder.name           # mandatory
        self.roll_number = builder.roll_number
        self.age = builder.age
        self.grade = builder.grade
        self.phone_number = builder.phone_number

    def __repr__(self):
        return (f"Student(name={self.name}, roll={self.roll_number}, "
                f"age={self.age}, grade={self.grade}, phone={self.phone_number})")

    class StudentBuilder:
        def __init__(self, name: str):
            self.name = name               # mandatory
            self.roll_number = 0
            self.age = 0
            self.grade = 0
            self.phone_number = ""

        def set_roll_number(self, roll_number: int):
            self.roll_number = roll_number
            return self

        def set_age(self, age: int):
            self.age = age
            return self

        def set_grade(self, grade: int):
            self.grade = grade
            return self

        def set_phone_number(self, phone_number: str):
            self.phone_number = phone_number
            return self

        def build(self):
            return Student(self)


if __name__ == "__main__":
    student = (Student.StudentBuilder("pradeep")
               .set_grade(2)
               .set_phone_number("9999999999")
               .build())
    print(student)
