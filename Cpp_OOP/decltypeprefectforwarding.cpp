#include <iostream>
#include <utility>
using namespace std;

void target(int& x) { cout << "接收左值：" << x << endl; }
void target(int&& x) { cout << "接收右值：" << x << endl; }

int main() {
    int a = 10;
    
    auto&& ref1 = a;          // 绑定左值 → 左值引用
    target(forward<decltype(ref1)>(ref1));  // 转发为左值 → 调用 target(int&)
    
    auto&& ref2 = 20;         // 绑定右值 → 右值引用
    target(forward<decltype(ref2)>(ref2));  // 转发为右值 → 调用 target(int&&)
    
    return 0;
}