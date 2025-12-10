#include <iostream>
#include <string>
#include <memory>  // 引入智能指针头文件

// 组件基类（咖啡）
class Coffee {
public:
    virtual std::string getDescription() = 0;
    virtual double getCost() = 0;
    virtual ~Coffee() = default;  // 虚析构函数确保子类正确析构
};
// 具体组件：基础咖啡
class BasicCoffee : public Coffee {
public:
    std::string getDescription() override {
        return "基础咖啡";
    }
    double getCost() override {
        return 10.0; // 基础咖啡10元
    }
};
// 装饰器基类（继承Coffee，包含Coffee智能指针）
class CoffeeDecorator : public Coffee {
protected:
    std::unique_ptr<Coffee> coffee;  // 替换为独占智能指针
public:
    // 构造函数接收智能指针，使用移动语义
    CoffeeDecorator(std::unique_ptr<Coffee> c) : coffee(std::move(c)) {}
    // 无需手动析构，智能指针会自动释放
    ~CoffeeDecorator() override = default;
};
// 具体装饰器1：加奶
class MilkDecorator : public CoffeeDecorator {
public:
    MilkDecorator(std::unique_ptr<Coffee> c) : CoffeeDecorator(std::move(c)) {}
    
    std::string getDescription() override {
        return coffee->getDescription() + " + 牛奶";
    }
    
    double getCost() override {
        return coffee->getCost() + 2.0; // 加奶加2元
    }
};

// 具体装饰器2：加糖
class SugarDecorator : public CoffeeDecorator {
public:
    SugarDecorator(std::unique_ptr<Coffee> c) : CoffeeDecorator(std::move(c)) {}
    
    std::string getDescription() override {
        return coffee->getDescription() + " + 糖";
    }
    
    double getCost() override {
        return coffee->getCost() + 1.0; // 加糖加1元
    }
};
int main() 
{
    // 基础咖啡（使用智能指针）
    std::unique_ptr<Coffee> coffee1 = std::make_unique<BasicCoffee>();
    std::cout << coffee1->getDescription() << "：" << coffee1->getCost() << "元" << std::endl;
    // 无需delete，咖啡1出作用域会自动释放
    // 基础咖啡+奶+糖（智能指针嵌套构造）
    std::unique_ptr<Coffee> coffee2 = 
        std::make_unique<SugarDecorator>(
            std::make_unique<MilkDecorator>(
                std::make_unique<BasicCoffee>()
            )
        );
    std::cout << coffee2->getDescription() << "：" << coffee2->getCost() << "元" << std::endl;
    // 无需delete，咖啡2出作用域会自动释放
    return 0;
}