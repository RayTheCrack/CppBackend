#include "smartpointer.hpp"
#include <iostream>

class MyObj
{
    private:

    int val;

    public:

    // 空构造（浅拷贝）
    explicit MyObj(const int& _val = 0) : val(_val) { std::cout << "Initialized the value : " << _val << std::endl; }
    // 移动构造
    MyObj(MyObj&& other) noexcept
    {
        std::cout << "Move initialized the value : " << val << std::endl;
        this->val = other.val;
        other.val = 0;
    }
    ~MyObj() { std::cout << "Released Resources\n";}
    void show() 
    {
        std::cout << val << std::endl;
    }
};

int main() 
{
    unique_ptr<MyObj> p = make_unique<MyObj>(20);
    p->show();
    // p.reset();
    if(p) std::cout << "true" << std::endl;
    else std::cout << "false" << std::endl;
    std::cout << std::endl;
    // 场景1：基本创建 + 引用计数
    shared_ptr<MyObj> p1 = make_shared<MyObj>(10);
    std::cout << "p1 计数：" << p1.get_count() << "，值：";
    p1->show();// 计数1，值10
    // 场景2：拷贝构造
    shared_ptr<MyObj> p2 = p1;
    std::cout << "p2 计数：" << p2.get_count() << std::endl; // 计数2
    std::cout << "p1 计数：" << p1.get_count() << std::endl; // 计数2

    // 场景3：拷贝赋值
    shared_ptr<MyObj> p3;
    p3 = p2;
    std::cout << "p3 计数：" << p3.get_count() << std::endl; // 计数3

    // 场景4：移动构造
    shared_ptr<MyObj> p4 = std::move(p1);
    std::cout << "p4 计数：" << p4.get_count() << std::endl; // 计数3
    std::cout << "p1 是否为空：" << (bool)p1 << std::endl; // false

    // 场景5：移动赋值
    shared_ptr<MyObj> p5;
    p5 = std::move(p4);
    std::cout << "p5 计数：" << p5.get_count() << std::endl; // 计数3
    std::cout << "p4 是否为空：" << (bool)p4 << std::endl; // false

    // 场景6：空指针
    shared_ptr<int> p6;
    std::cout << "p6 计数：" << p6.get_count() << std::endl; // 计数0

    // 场景7：资源释放（p2/p3/p5析构时，计数归0，MyObj析构）
    return 0;
}