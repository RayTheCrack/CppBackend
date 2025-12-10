#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

int main()
{
    int count = 0;
    auto countLambda = [&count]()
    {
        for(int i=1;i<=100000;i++) count++;
    };

    thread t1(countLambda);
    thread t2(countLambda);
    t1.join();
    t2.join();
    cout << "Final count is " << count << endl;
    // 为什么不是200000？ 
    // 因为两个线程在同时修改count变量，导致数据竞争，最终结果不可预测。
    // 解决方法是使用互斥锁（mutex）来保护对count变量的访问。
    // 下面是使用互斥锁的示例代码：
    count = 0; // 重置count
    mutex mtx;
    auto safeCountLambda = [&count, &mtx]()
    {
        for(int i=1;i<=100000;i++)
        {
            lock_guard<mutex> lock(mtx);
            count++;
        }
    };  
    thread t3(safeCountLambda);
    thread t4(safeCountLambda);
    t3.join();
    t4.join();
    cout << "Final safe count is " << count << endl;
    return 0;
}