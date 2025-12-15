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
};