#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
// 原子版本
std::atomic<int> atomic_cnt(0);
void atomic_inc() {
    for (int i = 0; i < 5000000; ++i) {
        atomic_cnt.fetch_add(1, std::memory_order_relaxed);
    }
}
// 锁版本
std::mutex mtx;
int mutex_cnt = 0;
void mutex_inc() {
    for (int i = 0; i < 5000000; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        mutex_cnt++;
    }
}
int main() {
    // 测试原子版本
    auto start = std::chrono::high_resolution_clock::now();
    std::thread t1(atomic_inc);
    std::thread t2(atomic_inc);
    t1.join();
    t2.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto atomic_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    // 测试锁版本
    start = std::chrono::high_resolution_clock::now();
    std::thread t3(mutex_inc);
    std::thread t4(mutex_inc);
    t3.join();
    t4.join();
    end = std::chrono::high_resolution_clock::now();
    auto mutex_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Atomic time: " << atomic_time << "ms, count: " << atomic_cnt << std::endl;
    std::cout << "Mutex time: " << mutex_time << "ms, count: " << mutex_cnt << std::endl;
    return 0;
}