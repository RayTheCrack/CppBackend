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
#include <utility>

class ThreadPool
{

private:

    struct compare
    {
        bool operator()(const std::pair<int, std::function<void()>>& a,const std::pair<int, std::function<void()>>& b)
        {
            return a.first < b.first;
        }
    };

    std::vector<std::thread> threads;
    std::priority_queue<std::pair<int, std::function<void()>>, std::vector<std::pair<int, std::function<void()>>>, compare> task_queue;
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
            std::pair<int,std::function<void()>> task = task_queue.top();
            task_queue.pop();
            lock.unlock();
            try {
                task.second(); // 执行任务
            } 
            catch (const std::exception& e) {
                std::cerr << "Task " << task.first << " exception: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Task " << task.first << " unknown exception" << std::endl;
            }
            task_count--;
            if(task_count == 0)
            {   
                // notify前需加锁，防止wait和notify间的竞态关系（还未阻塞就进行了notify）
                std::unique_lock<std::mutex> endlock(end_mtx);
                end_cv.notify_one();
            }
        }
    };  
public:

    std::atomic<int> task_count{0}; 
    std::condition_variable end_cv;
    std::mutex end_mtx;

    explicit ThreadPool(int ThreadNums) : stop(false)
    {
        for(int i=1;i<=ThreadNums;i++)
            threads.emplace_back(&ThreadPool::NewThread,this);
    };  

    ~ThreadPool() noexcept
    {
        {
            std::unique_lock<std::mutex> lock(mtx);
            stop.store(true);
        }   // 解锁（避免join时持有锁）
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
    void submit(int pri,F&& f,Args&& ...args)
    {
        // 完美转发保证传入函数f的参数值类型不变
        std::function<void()> func = std::bind(std::forward<F>(f),std::forward<Args>(args)...);
        {
            std::unique_lock<std::mutex> lock(mtx);               
            task_queue.emplace(pri,std::move(func));
            task_count++;
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
        pool.submit(i,[i]()
        {
            std::cout << "task " << i << " is running" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "task " << i << " is done" << std::endl;
        });
    }
    // 让所有任务先运行完，避免主程序提前退出析构线程池
    std::unique_lock<std::mutex> lock(pool.end_mtx);
    pool.end_cv.wait(lock,[&pool](){return pool.task_count == 0;});
    return 0;
}