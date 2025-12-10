#include <iostream>
#include <thread>
#include <chrono> // 用于sleep，放大风险场景

using std::cin;
using std::cout;
using std::endl;

int main()
{   
    // 核心：cnt是主线程的局部变量
    int cnt = 0;
    auto incre = [&cnt]()
    {
        // 线程休眠1秒，确保主线程先退出（放大风险）
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // 此时主线程已退出，cnt已被销毁，修改cnt会触发未定义行为
        for(int i=1; i<=1000000; i++) {
            cnt++;
        }
        // 尝试打印已销毁的变量，大概率输出乱码/程序崩溃
        cout << "Thread cnt: " << cnt << endl;
    }; 
    std::thread t1(incre);
    t1.detach(); // 分离线程，线程后台运行
    // 主线程不等待，直接退出（核心风险点）
    cout << "Main thread exit, cnt: " << cnt << endl;
    return 0; // 主线程退出，cnt被销毁，栈内存释放
}