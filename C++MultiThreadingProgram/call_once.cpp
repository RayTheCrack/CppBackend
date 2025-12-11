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

    // 私有析构函数：禁止外部删除（静态局部对象由系统自动析构）
    ~Singleton() 
    {
        std::cout << "[Destroy] Singleton destroyed! (Address: " << this << ")" << std::endl;
    }

    // 单例初始化函数：仅执行一次
    static void initInstance()
    {
        // 静态局部变量：C++11及以上保证线程安全初始化
        static Singleton UniqueInstance;
        instance = &UniqueInstance;
    }

    static std::once_flag flag;    // 保证initInstance仅执行一次
    static Singleton* instance;    // 单例对象指针

public:
    // 获取单例对象（线程安全）
    static Singleton* getInstance()
    {
        std::call_once(flag, initInstance);  // 一次性调用初始化函数
        return instance;
    }

    // 测试接口：打印当前线程ID
    void show()
    {
        std::cout << "[Thread] ID: " << std::this_thread::get_id() 
                  << " | Singleton Address: " << getInstance() << std::endl;
    }
};

// 静态成员初始化
Singleton* Singleton::instance = nullptr;
std::once_flag Singleton::flag;

// 线程执行函数
void test()
{
    Singleton::getInstance()->show();
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