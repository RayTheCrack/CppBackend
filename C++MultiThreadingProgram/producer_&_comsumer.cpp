#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
// 全局变量（简化示例，实际工程建议封装为类）
std::mutex mtx; // 保护队列
std::condition_variable cv; // 条件变量
std::queue<int> data_queue; // 数据队列
// 生产者线程：往队列放数据，通知消费者
void producer(int id) 
{
    for(int i=5;i>=0;i--) 
    {
        std::unique_lock<std::mutex> lock(mtx); // 加锁
        data_queue.push(i);
        std::cout << "Producer " << id <<  " 生产数据：" << i << std::endl;
        lock.unlock(); // 提前解锁（减少锁持有时间）
        cv.notify_one(); // 通知一个等待的消费者
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
// 消费者线程：消费数据
void consumer(int id) 
{
    while(true) 
    {
        std::unique_lock<std::mutex> lock(mtx);
        // 等待：队列非空时才能消费
        cv.wait(lock,[]() { return !data_queue.empty();});
        // 消费数据
        int data = data_queue.front();
        data_queue.pop();
        std::cout << "Consumer " << id << " consume: " << data << ", queue size: " << data_queue.size() << std::endl;
        // 通知生产者：队列有空位可生产
        cv.notify_one();
    }
}
int main() 
{
    // 创建2个生产者，2个消费者
    std::thread p1(producer,1);
    std::thread p2(producer,2);
    std::thread c1(consumer,1);
    std::thread c2(consumer,2);
    // 等待所有线程完成
    p1.join();
    p2.join();
    c1.join();
    c2.join();
    return 0;
}