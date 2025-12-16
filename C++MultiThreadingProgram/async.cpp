#include <iostream>
#include <future>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

int sum(int l,int r)
{
    int res = 0;
    for(int i=l;i<=r;i++)
    {
        res += i;
    }
    return res;
};
int main()
{
    std::future<int> fut = std::async(std::launch::async,sum,1,10);
    int ans = fut.get();
    std::cout << ans << std::endl;
    return 0;
}