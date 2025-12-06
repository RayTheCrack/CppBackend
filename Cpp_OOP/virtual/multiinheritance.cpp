#include <iostream>
using namespace std;

class Animal
{  
    public:
        int age;
        Animal(int x) : age(x)
        {
            cout << "Animal constructor called, age: " << age << endl;
        }
};

class Dog : virtual public Animal
{
    public:
        Dog(int x) : Animal(x)
        {
            cout << "Dog constructor called" << endl;
        }
};

class cat : virtual public Animal  
{   
    public:
        cat(int x) : Animal(x)
        {
            cout << "Cat constructor called" << endl;
        }
};

class pet : public Dog, public cat
{
    public:
        pet(int x) :  Animal(x), Dog(x), cat(x)
        {
            cout << "Pet constructor called" << endl;
        }
};

int main()
{
    pet obj(10);
    cout << obj.age << endl;
    
    return 0;
}