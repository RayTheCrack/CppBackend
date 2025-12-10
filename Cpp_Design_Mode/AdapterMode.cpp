#include <iostream>
#include <memory>
// 客户期望的接口（目标接口）
class Target {
public:
    virtual void request() = 0;
    virtual ~Target() = default;
};
// 被适配的类（已有接口，但不符合客户需求）
class Adaptee {
public:
    void specificRequest() {
        std::cout << "Adaptee: 执行特定操作（如输出220V）" << std::endl;
    }
};
// 适配器类（将Adaptee适配为Target接口）
class Adapter : public Target, private Adaptee {
public:
    void request() override {
        // 适配逻辑：调用Adaptee的方法，并转换接口
        std::cout << "Adapter: 适配转换（220V→5V）" << std::endl;
        specificRequest();
    }
};
// 测试
int main() 
{
    std::unique_ptr<Target> target = std::make_unique<Adapter>();
    // 客户只需调用Target的接口，无需关心底层实现
    target->request();
    return 0;
}