#include <iostream>
using namespace std;

class Person
{
    private:
        string name;
        int age;
    public:
        Person(string n, int a) : name(n), age(a) {}
        friend void displayPersonInfo(const Person& p);
        // void display(const Person& p);
};

void displayPersonInfo(const Person& p)
{
    cout << "Name: " << p.name << ", Age: " << p.age << endl;
}   

// void display(const Person& p)
// {
//     cout << "Name: " << p.name << ", Age: " << p.age << endl;
// }

int main()
{
    Person person("Alice", 30);
    displayPersonInfo(person); // Accessing private members via friend function
    // display(person);     // Accessing private members via member function
    return 0;
}