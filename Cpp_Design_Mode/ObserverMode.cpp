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
    std::vector<std::shared_ptr<Observer>> observers; // 观察者列表
public:
    // 添加观察者
    void attach(std::shared_ptr<Observer> obs)
    {
        observers.push_back(obs);
    }
    // 移除观察者
    void detach(std::shared_ptr<Observer> obs)
    {
        for (auto it = observers.begin(); it != observers.end(); ++it)
        {
            if (*it == obs)
            {
                observers.erase(it);
                break;
            }
        }
    }
    // 通知所有观察者
    void notify(const std::string &message)
    {
        for (auto &obs : observers)
        {
            obs->update(message);
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

public:
    User(const std::string &n) : username(n) {}
    void update(const std::string &message) override
    {
        std::cout << "用户[" << username << "]收到推送：" << message << std::endl;
    }
};
// 测试
int main()
{
    // 创建公众号
    std::shared_ptr<PublicAccount> account = std::make_shared<PublicAccount>("C++设计模式");
    // 创建用户
    std::shared_ptr<User> user1 = std::make_shared<User>("张三");
    std::shared_ptr<User> user2 = std::make_shared<User>("李四");
    // 用户订阅公众号
    account->attach(user1);
    account->attach(user2);
    // 发布文章
    account->publishArticle("C++设计模式详解");
    // 李四取消订阅
    account->detach(user2);
    // 再次发布文章
    account->publishArticle("C++多线程实战");
    return 0;
}