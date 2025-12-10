#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>

int main()
{   
    std::mutex mtx;
    auto print = [&mtx](int thread_id)
    {
        std::lock_guard<std::mutex> lock(mtx);
        for(int i=1;i<=100;i++)
        {
            std::cout << "Thread " << thread_id << ": printed " << i << std::endl;
        }
    };

    std::thread t1(print,1);
    std::thread t2(print,2);
    std::thread t3(print,3);
    t1.join();
    t2.join();
    t3.join();
    return 0;
}