#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex> // C++17及以上支持
#include <vector>
// 共享互斥锁
std::shared_mutex rw_mtx;
// 共享数据（多读少写）
int config_value = 100;
// 读线程：加读锁（共享）
void read_config(int thread_id) {
    // std::shared_lock 对应读锁
    std::shared_lock<std::shared_mutex> lock(rw_mtx);
    std::cout << "Reader " << thread_id << ": config_value = " << config_value << std::endl;
    // 析构时自动释放读锁
}
// 写线程：加写锁（独占）
void write_config(int new_value) {
    // std::unique_lock 对应写锁
    std::unique_lock<std::shared_mutex> lock(rw_mtx);
    config_value = new_value;
    std::cout << "Writer: update config_value to " << new_value << std::endl;
    // 析构时自动释放写锁
}
int main() {
    // 创建5个读线程（并发读取）
    std::vector<std::thread> readers;
    std::thread writer(write_config,200);
    for (int i = 1; i <= 5; ++i) {
        readers.emplace_back(read_config, i);
        if(i==5)
        {
            readers.back().join();
            writer.join();
            readers.pop_back();
        }
    }
    // 加短暂延迟，让写线程有机会先获取锁
    // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // 创建1个写线程（独占写入）
    for (auto& t : readers) {
        t.join();
    }
    return 0;
}