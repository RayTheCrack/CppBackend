#include <iostream>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
std::timed_mutex mtx;
int cnt;
int main()
{
    auto func = []()
    {
        for(int i=1;i<=2;i++)
        {
            std::unique_lock<std::timed_mutex> m(mtx,std::defer_lock);
            if(m.try_lock_for(std::chrono::seconds(1)))
            {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                cnt++;
            }
        }
    };
    std::thread t1(func);
    std::thread t2(func);
    t1.join();
    t2.join();
    std::cout << cnt << std::endl;
    return 0;
}