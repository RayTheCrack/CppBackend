#include <iostream>
#include <memory> // 智能指针的头文件
#include <stdexcept>
using namespace std;

class MyObj
{
    public:
        MyObj() { cout << "[Heap] MyObj 构造函数调用" << endl; }
        ~MyObj() { cout << "[Heap] MyObj 析构函数调用" << endl; }
        void display() { cout << "MyObj 的成员函数被调用" << endl; }
};

int main() 
{
    // auto p = make_unique<MyObj>(); // C++14 起支持
    cout << "Creating unique_ptr in stack..." << endl;
    unique_ptr<MyObj> p = make_unique<MyObj>();
    p->display();
    // 无需 delete，p析构时自动释放内存
    return 0;
}