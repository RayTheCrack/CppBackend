#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
using namespace std;

int main()
{
    atomic<int> cnt(0);
    auto count = [&cnt]()
    {
        for(int i=1;i<=10000;i++) cnt++;
    };
    thread t1(count);
    thread t2(count);
    t1.join();
    t2.join();
    cout << cnt << endl;
}