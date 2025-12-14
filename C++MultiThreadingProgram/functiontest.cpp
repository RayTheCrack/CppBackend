#include <iostream>
#include <functional>

int main()
{
    std::function<int(int,int)> add = [](int a,int b) -> int
    {
        return a + b;
    };
    std::cout << add(1,4) << std::endl;
    return 0;
}