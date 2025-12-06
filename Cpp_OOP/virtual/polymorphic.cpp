#include <iostream>
using namespace std;

class Animal
{
    public:
        virtual void speak() = 0; // Pure virtual function
};

class cat : public Animal
{
    public:
        void speak() override
        {
            cout << "Meow" << endl;
        }
};

class dog : public Animal
{
    public:
        void speak() override
        {
            cout << "Woof" << endl;
        }
};

void AnimalSound(Animal& animal)
{
    animal.speak();
}

int main()
{
    cat myCat;
    dog myDog;

    AnimalSound(myCat); // Outputs: Meow    
    AnimalSound(myDog); // Outputs: Woof
    
    return 0;
}