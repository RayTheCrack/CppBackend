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
    return 0;
}