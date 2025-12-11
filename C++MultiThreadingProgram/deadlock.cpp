#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>

std::mutex m1;
std::mutex m2;


int main()
{
    // deadlock :
    // auto func1 = []()
    // {
    //     std::unique_lock<std::mutex> lock1(m1);
    //     std::this_thread::sleep_for(std::chrono::milliseconds(500));
    //     std::unique_lock<std::mutex> lock2(m2);
    // };

    // auto func2 = []()
    // {
    //     std::unique_lock<std::mutex> lock1(m2);
    //     std::unique_lock<std::mutex> lock2(m1);
    // };

    auto func1 = []()
    {
        std::unique_lock<std::mutex> lock1(m1,std::defer_lock);
        std::unique_lock<std::mutex> lock2(m2,std::defer_lock);
        std::lock(lock1,lock2); // 保证了原子性同时加锁

    };

    auto func2 = []()
    {
        std::unique_lock<std::mutex> lock1(m1,std::defer_lock);
        std::unique_lock<std::mutex> lock2(m2,std::defer_lock);
        std::lock(lock1,lock2);

    };
    std::thread t1(func1);
    std::thread t2(func2);

    t1.join();
    t2.join();

    std::cout << "Over" << std::endl;
    return 0;
}