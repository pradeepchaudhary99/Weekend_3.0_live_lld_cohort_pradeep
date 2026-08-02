class Student:
    def __init__(self, builder: "Student.StudentBuilder"):
        self.name = builder.name
        self.roll_number = builder.roll_number
        self.age = builder.age
        self.grade = builder.grade
        self.phone_number = builder.phone_number

    class StudentBuilder:
        def __init__(self, name: str):
            self.name = name  # mandatory
            # optional fields
            self.roll_number = 11
            self.age = 12
            self.grade = 12
            self.phone_number = "dadsadasd"

        def set_roll_number(self, roll_number: int) -> "Student.StudentBuilder":
            self.roll_number = roll_number
            return self

        def set_age(self, age: int) -> "Student.StudentBuilder":
            self.age = age
            return self

        def set_grade(self, grade: int) -> "Student.StudentBuilder":
            self.grade = grade
            return self

        def set_phone_number(self, phone_number: str) -> "Student.StudentBuilder":
            self.phone_number = phone_number
            return self

        def build(self) -> "Student":
            return Student(self)


def main():
    student = (
        Student.StudentBuilder("pradeep")
        .set_grade(2)
        .set_phone_number("padsad")
        .build()
    )


if __name__ == "__main__":
    main()
