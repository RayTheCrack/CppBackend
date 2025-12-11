#include <iostream>
#include <vector>
#include <memory>
#include <string>
// 前向声明
class Subject;
// 观察者基类
class Observer
{
public:
    virtual void update(const std::string &message) = 0;
    virtual ~Observer() = default;
};

// 主题（被观察者）基类
class Subject
{
private:
    // 关键修改：用weak_ptr存储观察者，仅弱引用，不增加引用计数
    std::vector<std::weak_ptr<Observer>> observers; 
public:
    // 添加观察者：接收shared_ptr，转为weak_ptr存储
    void attach(std::shared_ptr<Observer> obs)
    {
        if (obs) { // 空指针检查，增强健壮性
            observers.push_back(obs);
        }
    }
    // 移除观察者：遍历weak_ptr列表，匹配对应的shared_ptr
    void detach(std::shared_ptr<Observer> obs)
    {
        for (auto it = observers.begin(); it != observers.end(); ++it)
        {
            auto spt = it->lock(); // 先声明变量，再判断
            if(spt && spt == obs)
            {
                observers.erase(it);
                break;
            }
        }
    }
    // 通知所有观察者：先检查观察者是否存活，再调用update
    void notify(const std::string &message)
    {
        // 遍历并清理失效的weak_ptr（观察者已被销毁）
        for (auto it = observers.begin(); it != observers.end();)
        {
            if (auto spt = it->lock()) { // lock成功表示观察者仍存活
                spt->update(message);
                ++it;
            } else {
                // 移除失效的weak_ptr，避免空引用
                it = observers.erase(it);
            }
        }
    }
    virtual ~Subject() = default;
};
// 具体主题：公众号
class PublicAccount : public Subject
{
private:
    std::string name;
public:
    PublicAccount(const std::string &n) : name(n) {}

    // 发布文章
    void publishArticle(const std::string &content)
    {
        std::cout << "\n公众号[" << name << "]发布了新文章：" << content << std::endl;
        notify(content); // 通知所有订阅者
    }
};
// 具体观察者：用户
class User : public Observer
{
private:
    std::string username;
    // 演示：若观察者需要持有主题，必须用weak_ptr（避免循环引用）
    std::weak_ptr<Subject> subject; 
public:
    // 构造函数：可选接收主题的weak_ptr
    User(const std::string &n, std::weak_ptr<Subject> sub = {}) 
        : username(n), subject(sub) {}
    void update(const std::string &message) override
    {
        std::cout << "用户[" << username << "]收到推送：" << message << std::endl;
        // 演示：观察者访问主题（需先lock检查是否存活）
        if (auto spt = subject.lock()) {
            std::cout << "用户[" << username << "]确认主题仍有效" << std::endl;
        }
    }
};
// 测试
int main()
{
    // 创建公众号（shared_ptr管理）
    std::shared_ptr<PublicAccount> account = std::make_shared<PublicAccount>("C++设计模式");
    // 创建用户：第二个用户持有主题的weak_ptr（演示循环引用场景）
    std::shared_ptr<User> user1 = std::make_shared<User>("张三");
    std::shared_ptr<User> user2 = std::make_shared<User>("李四", account);
    // 用户订阅公众号
    account->attach(user1);
    account->attach(user2);
    // 发布文章：两个用户都能收到
    account->publishArticle("C++设计模式详解");
    // 李四取消订阅
    account->detach(user2);
    // 再次发布文章：只有张三收到
    account->publishArticle("C++多线程实战");
    // 销毁李四（测试失效观察者自动清理）
    user2.reset();
    std::cout << "\n已销毁用户李四" << std::endl;
    // 第三次发布文章：仅张三存活，李四的weak_ptr被自动清理
    account->publishArticle("C++智能指针实战");
    return 0;
}