#include <iostream>
#include <string>
#include <memory>

// 产品基类
class Payment {
public:
    virtual void pay(double amount) = 0;
    virtual ~Payment() = default;
};
// 具体产品1：支付宝支付
class Alipay : public Payment {
public:
    void pay(double amount) override {
        std::cout << "支付宝支付：" << amount << "元" << std::endl;
    }
};
// 具体产品2：微信支付
class WeChatPay : public Payment {
public:
    void pay(double amount) override {
        std::cout << "微信支付：" << amount << "元" << std::endl;
    }
};
// 简单工厂类
class PaymentFactory {
public:
    static std::unique_ptr<Payment> createPayment(const std::string& type) {
        if (type == "alipay") {
            return std::make_unique<Alipay>();
        } else if (type == "wechat") {
            return std::make_unique<WeChatPay>();
        }
        throw std::invalid_argument("不支持的支付类型 : " + type);
    }
};
// 测试
int main() {
try
{
    // 创建支付宝支付对象
    std::unique_ptr<Payment> p1(PaymentFactory::createPayment("alipay"));
    p1->pay(100.5);
    // 创建微信支付对象
    std::unique_ptr<Payment> p2(PaymentFactory::createPayment("wechat"));
    p2->pay(200.0);
} 
catch(const std::invalid_argument& error)
{
    std::cout << "支付失败: " << error.what() << std::endl;
}
    return 0;
}