#include <iostream>
#include <utility>
using namespace std;

void print(int& x)
{
    cout << "LeftVal detected! Value : " << x << endl;
}

void print(int&& x)
{
    cout << "RightVal detected! Value : " << x << endl;
}

void print(const int& x)
{
    cout << "Const LeftVal detected! Value : " << x << endl;
}

template<class T>
void func(T&& param) // 万能引用（模板推导 + 引用折叠机制）
{
    print(forward<T>(param));
}

int main()
{
    int a = 10;
    func(a);
    func(100);
    int b = 112;
    func(b);
    func(move(b));
    const int con = 114514;
    func(con);
    return 0;
}