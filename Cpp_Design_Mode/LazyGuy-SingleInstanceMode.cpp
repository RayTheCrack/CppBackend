#include <iostream>
#include <mutex>
class Singleton {
private:
    // 私有化构造函数，禁止外部创建
    Singleton() { 
        std::cout << "Singleton 实例创建" << std::endl; 
    }
    // 禁用拷贝和赋值
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

public:
    // 全局访问点
    static Singleton& getInstance() {
        // C++11起，静态局部变量初始化是线程安全的
        static Singleton instance;
        return instance;
    }
    // 示例方法
    void doSomething() {
        std::cout << "单例对象执行操作" << std::endl;
    }
};
// 测试
int main() {
    // 获取唯一实例
    Singleton& s1 = Singleton::getInstance();
    Singleton& s2 = Singleton::getInstance();
    // 验证是同一个实例
    std::cout << "&s1: " << &s1 << std::endl;
    std::cout << "&s2: " << &s2 << std::endl;
    s1.doSomething();
    return 0;
}