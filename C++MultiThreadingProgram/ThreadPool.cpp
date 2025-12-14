#include <iostream>
#include <mutex>
#include <chrono>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <string>
#include <vector>
#include <functional>

class ThreadPool
{

private:

    std::vector<std::thread> threads;
    std::queue<std::function<void()>> task_queue;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> stop{false};
    void NewThread()
    {
        while(true)
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock,[this](){ return !task_queue.empty() or stop.load();});
            if(stop.load() and task_queue.empty()) 
                break;
            if(task_queue.empty())
                continue;
            std::function<void()> task = std::move(task_queue.front());
            task_queue.pop();
            lock.unlock();
            task();
        }
    };  
    
public:

    explicit ThreadPool(int ThreadNums) : stop(false)
    {
        for(int i=1;i<=ThreadNums;i++)
            threads.emplace_back(&ThreadPool::NewThread,this);
    };  

    ~ThreadPool() noexcept
    {
        std::unique_lock<std::mutex> lock(mtx);
        stop.store(true);
        cv.notify_all(); 
        for(auto &t : threads)
        {
            if(t.joinable())
                t.join();
        }
    }

    ThreadPool(const ThreadPool& other) = delete;
    ThreadPool& operator=(const ThreadPool& other) = delete;

    ThreadPool(ThreadPool&& other) noexcept = default;
    ThreadPool& operator=(ThreadPool&& other) noexcept = default;

    template<typename F,typename... Args>
    void submit(F&& f,Args&& ...args)
    {
        // 完美转发保证传入函数f的参数值类型不变
        std::function<void()> func = std::bind(std::forward<F>(f),std::forward<Args>(args)...);
        {
            std::unique_lock<std::mutex> lock(mtx);
            task_queue.emplace(std::move(func));
        }
        // 离开作用域后，lock已自动解锁析构
        cv.notify_one();
    }
};
int main()
{
    ThreadPool pool(5);
    for(int i=1;i<=10;i++)
    {
        pool.submit([i]()
        {
            std::cout << "task " << i << " is running" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "task " << i << " is done" << std::endl;
        });
    }
    // 给定100秒的时间让所有任务运行完，避免主程序提前退出析构线程池
    std::this_thread::sleep_for(std::chrono::seconds(100));
    return 0;
}