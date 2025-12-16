#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
using namespace std;

void change(int& num)
{
    num = 200;
}

int main()
{
    int num = 100;
    thread t1(change, ref(num));
    // t1.join();
    cout << "num = " << num << endl;

    std::this_thread::sleep_for(std::chrono::seconds(100));
    return 0;
}