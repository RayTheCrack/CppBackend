#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

void change(int& num)
{
    num = 200;
}

int main()
{
    int num = 100;
    thread t1(change, ref(num));
    t1.join();
    cout << "num = " << num << endl;
    return 0;
}