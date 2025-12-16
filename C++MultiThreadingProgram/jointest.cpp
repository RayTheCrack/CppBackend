#include <mutex>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

void thread_task(int id)
{
    std::cout << "Thread " << id << " is running!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Thread " << id << " finished!" << std::endl; 
}


int main()
{
    std::thread t1(thread_task,1);
    t1.join();
    std::thread t2(thread_task,2);
    t2.join();



    return 0;
}