#include <iostream>
#include <stdexcept>
#include <memory>

using namespace std;

void test()
{
    unique_ptr<int> ptr {make_unique<int>(42)};
    throw runtime_error("Intentional error for testing");
}

int main()
{
    try
    {
        test();
    }
    catch (const runtime_error& e)
    {
        cout << "Caught an exception: " << e.what() << endl;
    }
    // 资源已释放，无泄漏；但函数未完成预期操作（符合基本保证）
    return 0;
}