#include <iostream> 
using namespace std;

class Base
{
    public:
        virtual void func()
        {
            cout << "Function has been called!" << endl;
        } 
};

class Son : public Base
{
    public:
        void func() override 
        {
            cout << "Function has been called in Class Son" << endl;
        }
};

int main()
{
    Base b;
    Son s;
    b.func();
    s.func();
    return 0;
}