#include <iostream>
#include <memory>
using namespace std;

int main() 
{
    // 1. 创建 shared_ptr
    shared_ptr<int> sp = make_shared<int>(100);
    // 2. 创建 weak_ptr，关联到 sp
    weak_ptr<int> wp = sp;
    // 3. 查看状态
    cout << "是否过期：" << boolalpha << wp.expired() << endl; // false
    cout << "计数：" << wp.use_count() << endl; // 1
    // 4. 转换成 shared_ptr 访问资源
    if (shared_ptr<int> temp = wp.lock()) { // 资源未过期，temp 有效
        cout << *temp << endl; // 输出 100
    } else {
        cout << "资源已释放" << endl;
    }
    // 5. 释放 sp，资源销毁
    sp.reset(); // 手动重置 shared_ptr，计数减到 0
    cout << "是否过期：" << wp.expired() << endl; // true
    // 6. 再次尝试访问（失败）
    if (shared_ptr<int> temp = wp.lock()) {
        cout << *temp << endl;
    } else {
        cout << "资源已释放" << endl; // 输出这行
    }
    return 0;
}