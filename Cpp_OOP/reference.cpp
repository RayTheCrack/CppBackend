#include <iostream> 
using namespace std;

int main()
{
    int i = 10;
    int& r = i;
    cout << i << ' ' << r << endl;
    auto swap = [&](int& x,int& y)
    {
        int temp = x;
        x = y;
        y = temp;
    };
    int a = 10, b = 20;
    swap(a,b);
    cout << a << ' ' << b << endl;
    return 0;
}