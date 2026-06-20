
package DesignPattern.CreationalDesignPattern;


class Student{
    private String name; //mandatory

    // optional fields
    private int rollNumber = 11;
    private long age = 12;
    private int grade = 12;
    private String phoneNumber = "dadsadasd";


    // lets stop object creation directly
    private Student(StudentBuilder builder){
        this.name  = builder.name;
        this.rollNumber = builder.rollNumber;
        this.age   = builder.age;
        this.grade =  builder.grade;
        this.phoneNumber = builder.phoneNumber;
    }

    static class StudentBuilder{
        private String name; //mandatory
        // optional fields
        private int rollNumber;
        private long age;
        private int grade;
        private String phoneNumber;   

        public StudentBuilder(String name){
            this.name = name;
        }

        StudentBuilder setRollNumber(int rollNumber){
            this.rollNumber = rollNumber;
            return this;
        }

        StudentBuilder setAge(int age){
            this.age = age;
            return this;
        }

        StudentBuilder setGrade(int grade){
            this.grade = grade;
            return this;
        }

        StudentBuilder setPhoneNumber(String phoneNumber){
            this.phoneNumber = phoneNumber;
            return this;
        }

        Student build(){
            return new Student(this);
        }
    }
}

public class BuilderDesign{

    public static void main(String[] args) {
    
        Student student = new Student.StudentBuilder("pradeep").setGrade(2).setPhoneNumber("padsad").build();

        
    
    }
}