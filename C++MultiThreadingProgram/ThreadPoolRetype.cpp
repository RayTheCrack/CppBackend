// Basic ThreadPool
// Write By @OxyTheCrack 2025.12.15

#include <iostream>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <memory>
#include <thread>
#include <utility>
#include <queue>
#include <atomic>
#include <functional>

class ThreadPool
{

private:

    std::mutex mtx;
    std::queue<std::function<void()>> task_queue;
    std::vector<std::thread> threads;
    std::atomic<bool> stop{false}; 
    std::condition_variable cv;
    
    void NewThread()
    {
        while(true)
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock,[this](){return !task_queue.empty() or stop.load();});
            if(stop.load() and task_queue.empty())
                break;
            if(task_queue.empty())
                continue;
            std::function<void()> task = std::move(task_queue.front());
            task_queue.pop();
            lock.unlock();
            task();
            task_count--;
            if(task_count == 0)
            {
                std::unique_lock<std::mutex> endlock(end_mtx);
                end_cv.notify_one();
            }
        }
    }

public:

    std::atomic<int> task_count{0};
    std::condition_variable end_cv;
    std::mutex end_mtx;

    explicit ThreadPool(int ThreadsCnt) : stop(false)
    {
        for(int i=1;i<=ThreadsCnt;i++) 
        {
            threads.emplace_back(&ThreadPool::NewThread,this);
        }
    };  

    ThreadPool(const ThreadPool& other) = delete;
    ThreadPool& operator=(const ThreadPool& other) = delete;

    ThreadPool(ThreadPool&& other) noexcept = default;
    ThreadPool& operator=(ThreadPool&& other) noexcept = default;

    ~ThreadPool() noexcept
    {
        {
            std::unique_lock<std::mutex> lock(mtx);
            stop.store(true);
        }
        cv.notify_all();
        for(auto &t : threads)
        {
            if(t.joinable())
                t.join();
        }
    }

    template<typename F,typename...Args>
    void submit(F&& f,Args&& ...args)
    {
        std::function<void()> task = std::bind(std::forward<F>(f),std::forward<Args>(args)...);
        {
            std::unique_lock<std::mutex> lock(mtx);
            task_queue.emplace(std::move(task));
            task_count++;
        }
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
    // 先让所有任务运行完，避免主程序提前退出析构线程池
    std::unique_lock<std::mutex> endlock(pool.end_mtx);
    pool.end_cv.wait(endlock,[&pool](){return pool.task_count == 0;});
    return 0;
}