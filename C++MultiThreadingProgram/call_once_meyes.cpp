#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include <future>  // 确保std::once_flag正常工作（部分环境依赖）

using namespace std;

class Singleton
{
private:
    // 私有构造函数：禁止外部实例化
    Singleton()
    {
        std::cout << "[Init] Created A Singleton! (Address: " << this << ")" << std::endl;
    }

    // 禁用拷贝构造和赋值运算符：防止单例被复制
    Singleton(const Singleton& other) = delete;
    Singleton& operator=(const Singleton& other) = delete;

public:


    static Singleton& getInstance()
    {
        static Singleton UniqueInstance;
        return UniqueInstance;
    }

    // 测试接口：打印当前线程ID
    void show()
    {
        std::cout << "[Thread] ID: " << std::this_thread::get_id() 
                  << " | Singleton Address: " << &getInstance() << std::endl;
    }
};

// 线程执行函数
void test()
{
    Singleton::getInstance().show();
}

int main()
{
    // 创建5个线程测试单例
    thread t1(test);
    thread t2(test);
    thread t3(test);
    thread t4(test);
    thread t5(test);

    // 等待所有线程结束
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    std::cout << "\n[Main] All threads finished. Singleton is unique." << std::endl;

    return 0;
}