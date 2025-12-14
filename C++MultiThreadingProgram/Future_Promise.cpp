#include <iostream>
#include <thread>
#include <future> // 包含future/promise
// 生产者函数：计算结果并设置到promise
void calculate_sum(std::promise<int>& prom, int a, int b) 
{
    std::this_thread::sleep_for(std::chrono::seconds(1)); // 模拟耗时计算
    int sum = a + b;
    prom.set_value(sum); // 设置结果，future可获取
}
int main() 
{
    // 创建promise对象
    std::promise<int> sum_prom;
    // 获取对应的future对象
    std::future<int> sum_fut = sum_prom.get_future();
    // 创建线程，传递promise
    std::thread t(calculate_sum,std::ref(sum_prom),10,20);
    // 主线程获取结果（阻塞直到线程设置结果）
    std::cout << "Waiting for result..." << std::endl;
    int result = sum_fut.get(); // get()只能调用一次，调用后future变为无效
    std::cout << "Sum: " << result << std::endl; // 输出30
    t.join();
    return 0;
}