#include <iostream>
#include <thread>
#include <future>
int calculate_mul(int a,int b) 
{
    return a * b;
}
int main()
{
    std::packaged_task<int(int,int)> task(calculate_mul);
    std::future<int> fut = task.get_future();
    // int temp = fut.get();
    std::thread t(std::move(task),20,10);
    int res = fut.get();
    t.join();
    std::cout << res << std::endl;
    

    return 0;
}