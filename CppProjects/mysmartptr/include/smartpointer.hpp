#ifndef SMARTPOINTER_HPP
#define SMARTPOINTER_HPP

#include <iostream>
#include <utility> // std::move
#include <cstddef> // size_t

template<typename T>
class unique_ptr
{
    private:
        T* ptr;
    public:
        // 空构造
        explicit unique_ptr(T* p = nullptr) : ptr(p) {}

        // 移动构造函数
        unique_ptr(unique_ptr&& other) noexcept
        {
            this->ptr = other.ptr;
            other.ptr = nullptr; // 转移所有权
        }

        // 移动赋值运算符，赋值将亡值的资源
        unique_ptr& operator=(unique_ptr&& other) noexcept
        {
            if(this != &other)
            {
                // 释放当前指针指向的内存
                if(ptr)
                {
                    delete ptr;
                }
                // 转移所有权
                ptr = other.ptr;
                other.ptr = nullptr;
            }
            return *this;
        }

        // 析构函数，动态释放资源

        virtual ~unique_ptr noexcept
        {
            if(ptr)
            {
                // 释放当前指针指向的内存
                delete ptr;
                // 当前指针置为空
                ptr = nullptr;
            }
        }
        
        // 禁用拷贝构造函数和拷贝赋值运算符（unique_ptr不允许拷贝）
        // 必须显示删除拷贝构造函数和拷贝赋值运算符，不删除的话，编译器会自动生成默认的拷贝构造函数和拷贝赋值运算符
        // 这样会导致多个unique_ptr实例指向同一块内存，最终会导致双重释放内存的问题
        
        unique_ptr(const unique_ptr&) = delete;
        unique_ptr& operator=(const unique_ptr&) = delete;

        // 指针操作

        // 解引用，访问对象
        T& operator*() const noexcept 
        {
            return *ptr;
        }   

        // 成员访问：访问对象成员
        T* operator->() const noexcept
        {
            return ptr;
        }

        // 获取裸指针（慎用）
        T* get() const noexcept
        {
            return ptr;
        }

        // 手动释放资源

        void reset(T* otherptr = nullptr) noexcept
        {
            if(ptr)
            {
                delete ptr;
            }
            ptr = otherptr;
        }

        // 布尔转换 ：判断是否持有资源
        // 布尔转换运算符是 C++ 中的一种用户定义类型转换 (user-defined conversion)
        // 它允许你自定义一个类的对象在需要被当作布尔值使用时（比如在 if、while 条件判断中），应该如何转换为 bool 类型。
        explicit operator bool() const noexcept
        {
            return ptr != nullptr;
        }
};

template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) 
{
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif // SMARTPOINTER_HPP