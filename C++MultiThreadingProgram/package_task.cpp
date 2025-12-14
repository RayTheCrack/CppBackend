#include <iostream>
#include <thread>
#include <future>
int calculate_mul(int a,int b) 
{
    return a * b;
}
int main()
{
    // 包装函数，关联future（返回值类型int）
    std::packaged_task<int(int, int)> task(calculate_mul);
    // 获取future
    std::future<int> fut = task.get_future();
    // 创建线程，传递task（必须移动）
    std::thread t(std::move(task),5,6);
    // 获取结果
    int result = fut.get();
    std::cout << "Mul: " << result << std::endl; // 输出30
    t.join();
    return 0;
}