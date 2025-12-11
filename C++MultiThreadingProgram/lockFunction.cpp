#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>
std::mutex m1;
std::mutex m2;

static size_t ID = 0;

class BankAccount
{
private:

    double balance;
    size_t usrID;
    std::string usrName;

public:

    std::mutex mtx;

    BankAccount(const BankAccount& other) = delete;
    BankAccount& operator=(const BankAccount& other) = delete;

    BankAccount(std::string& _Name, double _balance = 0.0) : usrID(++ID), balance(_balance), usrName(_Name) {}
    BankAccount(std::string&& _Name, double _balance = 0.0) : usrID(++ID), balance(_balance), usrName(std::move(_Name)) {}

    void printBalance() 
    {
        std::cout << "[ID]"<< usrID << ":" <<usrName << " has balance of : " << balance << "!\n";
    }

    void out(double amount)
    {
        this->balance -= amount;
    }

    void in(double amount)
    {
        this->balance += amount;
    }
};

void Transfer(BankAccount& from,BankAccount& to,double amount)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::unique_lock<std::mutex> lockfrom(from.mtx,std::defer_lock);
    std::unique_lock<std::mutex> lockto(to.mtx,std::defer_lock);
    std::lock(lockfrom,lockto);

    // from.mtx.lock();
    // to.mtx.lock();
    //std::lock(from.mtx,to.mtx);

    from.out(amount);
    to.in(amount);

    // from.mtx.unlock();
    // to.mtx.unlock();
};  

int main()
{

    BankAccount usr1("张三");
    BankAccount usr2("李四");

    std::thread t1(Transfer,std::ref(usr1),std::ref(usr2),100);
    std::thread t2(Transfer,std::ref(usr2),std::ref(usr1),200);

    t1.join();
    t2.join();

    usr1.printBalance();
    usr2.printBalance();

    std::cout << "Over" << std::endl;
    return 0;
}