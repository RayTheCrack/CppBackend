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
                // if(ptr) 没必要加，因为delete nullptr也是合法行为
                delete ptr;
                // 转移所有权
                ptr = other.ptr;
                other.ptr = nullptr;
            }
            return *this;
        }

        // 析构函数，动态释放资源
        // 一定不会被继承的类，无需定义析构函数为虚函数，避免创建vptr,vtable的额外开销
        ~unique_ptr() noexcept
        {
            // 释放当前指针指向的内存
            delete ptr;
            // 当前指针置为空
            ptr = nullptr;
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
            delete ptr; 
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

template<typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args)
{
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<typename T> 
class shared_ptr
{
    private:
        T* ptr;
        size_t* shared_count;

        void release() noexcept
        {
            // 一定要先判断指针是否为空，对空指针解引用会直接崩溃
            if(shared_count)
            {
                (*shared_count)--;

                if(*shared_count == 0)
                {
                    // new 出的资源需要删除
                    delete ptr;
                    delete shared_count;
                    this->ptr = nullptr;
                    this->shared_count = nullptr;
                } 
            }
        };
    public:
        explicit shared_ptr(T* p = nullptr) noexcept : ptr(p), shared_count(nullptr)
        {
            if(ptr)
            {
                shared_count = new size_t(1);
            }
        }

        // 拷贝构造，shared_count++
        shared_ptr(const shared_ptr<T>& other)
        {
            this->ptr = other.ptr;
            this->shared_count = other.shared_count;
            if(shared_count) // 持有有效资源时再++
                (*shared_count)++;
        }
        // 移动构造，接管将亡值资源，原对象置空
        shared_ptr(shared_ptr<T>&& other) noexcept
        {
            this->ptr = other.ptr;
            this->shared_count = other.shared_count;
            other.ptr = nullptr;
            other.shared_count = nullptr;
        }
        // 析构函数
        ~shared_ptr() noexcept
        {
            this->release();
        }
        // 拷贝赋值运算符，先释放自己的资源，再拷贝other的资源
        shared_ptr<T>& operator=(const shared_ptr<T>& other)
        {
            // this 为当前类存储的地址 如果等于 other类存储的地址，说明此时产生自赋值
            if(this == &other) return *this; // 要避免自赋值，此时不进行任何操作
        
            this->release();

            this->ptr = other.ptr;
            this->shared_count = other.shared_count;

            if(shared_count) 
                (*shared_count)++;

            return *this;
        } 
        // 移动赋值运算符
        shared_ptr<T>& operator=(shared_ptr<T>&& other) noexcept
        {
            // this 为当前类存储的地址 如果等于 other类存储的地址，说明此时产生自赋值
            if(this == &other) return *this; // 要避免自赋值，此时不进行任何操作

            this->release();

            this->ptr = other.ptr;
            this->shared_count = other.shared_count;

            other.ptr = nullptr;
            other.shared_count = nullptr;

            return *this;
        } 

        // 指针操作
        T* operator->() const noexcept
        {
            return this->ptr;
        }

        T* get() const noexcept
        {
            return this->ptr;
        }

        T& operator*() const noexcept
        {
            return *(this->ptr);
        }

        void reset() noexcept
        {
            this->release();            
        }
        
        // 布尔转换
        explicit operator bool() const noexcept
        {
            return this->ptr != nullptr;
        }

        size_t get_count() const noexcept
        {
            return shared_count ? *shared_count : 0;
        }

};

template<typename T,typename... Args>
shared_ptr<T> make_shared(Args&&... args)
{
    return shared_ptr<T>(new T(std::forward<Args>(args)...));
}


#endif // SMARTPOINTER_HPP