#include <iostream>
using namespace std;

class Teacher; // 前置声明

class Student
{
    private:
        string name;
        int age;
    public:
        Student(string n, int a) : name(n), age(a) {}
        friend class Teacher; // 将 Teacher 类声明为友元类
};

class Teacher
{
    public:
        void displayStudentInfo(const Student& s)
        {
            // 访问 Student 类的私有成员
            cout << "Student Name: " << s.name << ", Age: " << s.age << endl;
        }
        void changeStudentAge(Student& s, int newAge)
        {
            s.age = newAge; // 修改 Student 类的私有成员
        }
};

int main()
{
    Student student("Bob", 20);
    Teacher teacher;

    teacher.displayStudentInfo(student); // 访问私有成员
    teacher.changeStudentAge(student, 21); // 修改私有成员
    teacher.displayStudentInfo(student); // 再次访问以显示更改后的信息

    return 0;
}