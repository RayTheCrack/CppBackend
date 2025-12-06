#include <iostream>
#include <stdexcept>

using namespace std;

int divide(int x,int y)
{
    if(y == 0) throw runtime_error("Division by zero error");
    else return x / y;
}

int add(int x,int y) noexcept 
{
    return x + y;
}

int mul(int x,int y) noexcept
{
    return x * y;
}

int main()
{
    int x,y;
    cin >> x >> y;
    try 
    {
        int result = divide(x,y);
        cout << "Result: " << result << endl;
    }
    catch (const runtime_error& e) 
    {
        cout << "Caught an exception: " << e.what() << endl;
    }
}