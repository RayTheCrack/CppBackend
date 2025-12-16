# Smart_Pointer

## 项目整体结构（模块化设计）

整个项目分为 3 个核心文件，符合 C++ 工程化规范，避免所有代码堆在一个文件里：

| 文件名称        | 作用                                                         |
| :-------------- | :----------------------------------------------------------- |
| `smart_ptr.hpp` | 头文件：声明 `UniquePtr`/`SharedPtr` 模板类，定义核心接口    |
| `smart_ptr.cpp` | 源文件：实现模板类的非内联成员函数（可选，模板类也可全放头文件） |
| `main.cpp`      | 测试文件：编写测试用例，验证智能指针的功能和异常场景         |

**补充：模板类的文件规范**

C++ 模板类的实现通常直接放在头文件中（因为模板实例化需要看到完整实现），可以把所有代码写在 `smart_ptr.hpp` 里，简化结构；后续熟悉后再拆分到 `.cpp`（需显式实例化）。

## 实现简化`unique_ptr`

**核心特性**

- 独占资源所有权，**禁止拷贝**（拷贝构造 / 赋值 `delete`）；
- 支持**移动语义**（转移资源所有权）；
- `RAII` 自动释放资源（析构函数 `delete` 指针）；
- 重载 `*`/`->` 运算符，支持指针式访问；
- 标记关键函数 `noexcept`（保证异常安全）。

```c++
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
```

### `explicit` 关键字

`explicit`是 C++ 中的一个关键字，**只能修饰单参数的构造函数（或除第一个参数外其他参数都有默认值的多参数构造函数）**，它的核心作用是：**禁止构造函数进行隐式类型转换**，强制要求必须显式调用构造函数创建对象。

先通过一个例子理解 “隐式转换” 是什么，以及为什么需要`explicit`来限制：

```cpp
#include <iostream>
using namespace std;
class MyInt {
private:
    int value;
public:
    // 单参数构造函数（无explicit）
    MyInt(int v) : value(v) {}
    
    void print() {
        cout << "值为：" << value << endl;
    }
};
// 测试函数：接收MyInt类型参数
void testFunc(MyInt num) {
    num.print();
}
int main() {
    // 隐式转换：编译器自动将int类型的10转换为MyInt对象
    testFunc(10); 
    return 0;
}
```

**运行结果**：`值为：10`

这个例子中，`testFunc`需要`MyInt`类型的参数，但你传入了`int`类型的`10`，编译器会自动调用`MyInt(int v)`构造函数，把`10`隐式转换成`MyInt`对象。这种隐式转换在某些场景下会导致代码逻辑不清晰，甚至出现意想不到的错误。

给构造函数加上`explicit`后，上面的隐式转换会被禁止：

```cpp
#include <iostream>
using namespace std;
class MyInt {
private:
    int value;
public:
    // 单参数构造函数（加explicit）
    explicit MyInt(int v) : value(v) {}
    
    void print() {
        cout << "值为：" << value << endl;
    }
};
void testFunc(MyInt num) {
    num.print();
}
int main() {
    // 错误：无法从int隐式转换为MyInt
    // testFunc(10); 
    // 正确：显式调用构造函数创建对象
    testFunc(MyInt(10)); 
    return 0;
}
```

**运行结果**：`值为：10`

此时编译器会拒绝隐式转换，必须显式写出`MyInt(10)`才能创建对象，代码的意图更清晰。

#### `explicit`的使用场景

1. **避免意外的隐式转换**：比如数值类型、字符串类型与自定义类的转换，防止代码逻辑偏离预期；
2. **提升代码可读性**：显式构造让代码的意图更明确，其他开发者能一眼看出对象的创建方式；
3. **`STL` 中的应用**：比如`std::string`、`std::vector`的构造函数，部分场景会用`explicit`限制隐式转换（如`std::vector(size_t n)`）。

#### 总结

1. `explicit`仅修饰**单参数构造函数**（或除第一个参数外其他参数有默认值的多参数构造函数）；
2. 核心作用是**禁止构造函数的隐式类型转换**，强制显式创建对象；
3. 主要价值是避免意外转换、提升代码可读性，是 C++ 中规范代码的常用技巧。

### 移动构造 与 移动赋值运算符

#### 移动构造函数

移动构造函数是一种特殊的构造函数，用于**用一个 “即将被销毁” 的右值对象（`rvalue`）初始化新对象**，直接接管原对象的动态资源，而非拷贝。

```c++
class 类名 {
public:
    // 移动构造函数（核心：参数是类的右值引用 T&&）
    类名(类名&& other) noexcept {
        // 1. 接管other的资源
        this->ptr = other.ptr;
        // 2. 将other置为“空/有效”状态（避免析构时重复释放）
        other.ptr = nullptr;
    }
    // 对比：拷贝构造函数（参数是const左值引用 const T&）
    // 拷贝构造：初始化一个新的类对象，使这个新对象的资源是另一个已有对象资源的 “独立副本”；
    类名(const 类名& other) {
        // 拷贝资源（比如重新分配内存，复制数据）
        this->ptr = new int(*other.ptr);
    }
};
```

- 参数必须是**右值引用**（`T&&`）：右值引用专门绑定到临时对象、即将销毁的对象等 “右值”；
- 通常加`noexcept`：告诉编译器该函数不会抛出异常（容器如`std::vector`在扩容时，若移动构造有异常会退化为拷贝，加`noexcept`才能保证优化生效）；
- 核心逻辑：接管资源 + 置空原对象（避免原对象析构时释放已转移的资源）。

#### 移动赋值运算符

移动赋值运算符是赋值运算符的重载版本，用于**将一个右值对象的资源转移给已存在的对象**（区别于移动构造：移动构造是初始化新对象，移动赋值是给已有对象赋值）。

```cpp
class 类名 {
public:
    // 移动赋值运算符
    类名& operator=(类名&& other) noexcept {
        // 0. 防止自赋值（虽然右值自赋值概率极低，但仍需防御）
        if (this == &other) {
            return *this;
        }
        // 1. 释放当前对象的已有资源（避免内存泄漏）
        delete[] this->m_data;
        // 2. 接管other的资源
        this->m_data = other.m_data;
        this->m_size = other.m_size;
        // 3. 置空other
        other.m_data = nullptr;
        other.m_size = 0;
        return *this;
    }
    // 对比：拷贝赋值运算符
    // 拷贝赋值：更新一个已存在的类对象，使这个对象的资源替换为另一个已有对象资源的 “独立副本”（先释放旧资源，再复制新资源）。
    类名& operator=(const 类名& other) {
        if (this == &other) return *this;
        delete[] this->m_data;  // 释放当前资源
        m_size = other.m_size;
        m_data = new char[m_size + 1];  // 重新分配
        strcpy(m_data, other.m_data);   // 拷贝数据
        return *this;
    }
};
```

**关键细节：右值引用与 std::move**

- 什么时候会自动触发移动？

只有当源对象是**右值**（临时对象、匿名对象、即将销毁的对象）时，编译器才会自动调用移动构造 / 赋值。

- 如何强制移动左值？

如果想对**左值**（有名字的对象）触发移动，可以用`std::move`将其转为右值引用:

```c++
MyString s1("Test");
// s1是左值，默认调用拷贝构造；std::move后转为右值，调用移动构造
MyString s2 = std::move(s1);  
s1.print();  // 输出：空对象（s1的资源已被转移）
s2.print();  // 输出：Test
```

注意：`std::move`只是 “标记” 对象可以被移动，不会真的移动资源；移动后原对象会处于 “有效但未定义” 的状态（通常置空），**不能再使用原对象的资源**（但可以析构）。

使用场景与最佳实践

1. **必须实现移动的场景**：类管理动态资源（堆内存、文件句柄、网络连接）时，实现移动构造 / 赋值能大幅提升性能；
2. **加`noexcept`**：移动构造 / 赋值几乎都应加`noexcept`，否则容器（如`std::vector`）会放弃移动、改用拷贝；
3. **移动后置空原对象**：必须将原对象的资源指针置空，避免析构时重复释放；
4. **避免滥用 std::move**：仅对不再使用的对象使用`std::move`，否则会导致原对象资源失效。

总结

1. 移动构造 / 移动赋值的核心是**转移资源所有权**，而非拷贝，目的是优化性能、避免不必要的内存分配；
2. 移动构造用于**初始化新对象**，移动赋值用于**给已有对象赋值**，两者参数都是右值引用（`T&&`），且通常加`noexcept`；
3. 右值自动触发移动，左值需用`std::move`转为右值；移动后原对象资源应置空，不可再使用。

### 编译器的默认行为

- 如果类中**没有自定义**拷贝构造、拷贝赋值、移动构造、移动赋值、析构函数，编译器会**自动生成默认的移动构造 / 移动赋值**（浅拷贝，仅拷贝指针等成员）；
- 如果类中**自定义了拷贝构造 / 拷贝赋值 / 析构**，编译器**不会自动生成**移动构造 / 移动赋值（此时类会退化为 “只能拷贝，不能移动”）；
- 可以用`= default`显式要求编译器生成默认移动构造 / 赋值，用`= delete`禁用移动：

```cpp
// 显式生成默认移动构造
MyString(MyString&& other) noexcept = default;
// 禁用拷贝赋值 和 拷贝赋值运算符 （unique_pointer不允许拷贝）
unique_ptr(const unique_ptr&) = delete;
unique_ptr& operator=(const unique_ptr&) = delete;
```

### `noexcept`前的`const`修饰词的作用

`operator*() const noexcept` 中，`noexcept` 前的 `const` 作用是：**声明这个成员函数不会修改所属类对象的任何成员变量（除非成员变量被 `mutable` 修饰），且只能被 `const` 修饰的类对象调用**。

这个 `const` 和返回值类型（`T&`/`const T&`）、`noexcept` 是完全独立的三个维度：

- `const` → 约束 “函数是否能修改对象本身”
- 返回值 → 约束 “通过函数能如何操作指向的对象”
- `noexcept` → 声明 “函数不会抛出异常”

**`const` 修饰成员函数的核心规则**

- `const` 成员函数不能修改类的成员变量

在 `const` 成员函数内部，编译器会把 `this` 指针的类型从 `T* const` 变成 `const T* const`—— 也就是说，`this` 指针指向的对象被视为 `const`，因此无法修改任何非 `mutable` 的成员变量。

```c++
template <typename T>
class SmartPtr {
private:
    T* ptr; // 普通成员变量
public:
    // const 成员函数
    T& operator*() const noexcept {
        ptr = new T(); // ❌ 编译报错！const 成员函数不能修改成员变量 ptr
        return *ptr;
    }
    // 非 const 成员函数
    T& operator*() noexcept {
        ptr = new T(); // ✅ 编译通过！可以修改成员变量 ptr
        return *ptr;
    }
};
```

这就是 `const` 的核心约束：**保证这个运算符重载不会改变智能指针本身的状态（比如不会让 `ptr` 指向别的地址）**，符合 “解引用指针只是访问对象，不会修改指针本身” 的直觉

- **`const` 成员函数的调用权限**

`const` 修饰的类对象 → **只能调用 `const` 成员函数**（不能调用非 `const` 成员函数）；

非 `const` 修饰的类对象 → **可以调用 `const`成员函数 + 非 `const` 成员函数**（优先调用非 `const` 版本，如果有）。

| 类对象类型         | 能调用的成员函数类型          | 举例（智能指针）                                             |
| ------------------ | ----------------------------- | ------------------------------------------------------------ |
| `const` 智能指针   | 仅 `const` 成员函数           | `const SmartPtr<int> sp; *sp;` → 只能调用 `operator*() const` |
| 非 `const`智能指针 | `const` + 非 `const` 成员函数 | `SmartPtr<int> sp; *sp;` → 有非 `const` 版本就调它，没有就调 `const` 版本 |

如果只写了 `operator*() const`，那么不管是 `const` 还是非 `const` 智能指针，都会调用这个版本 —— 因为非 `const` 对象可以调用 `const` 成员函数，只是反过来不行（`const` 对象不能调用非 `const` 成员函数）。

- **`const` 成员函数只能调用其他 `const` 成员函数**

在 `const` 成员函数内部，如果你调用类的其他成员函数，只能调用那些也是 `const` 修饰的成员函数，不能调用非 `const` 成员函数（避免间接修改对象状态）。

```c++
template <typename T>
class SmartPtr {
private:
    T* ptr;
    void reset() { // 非 const 成员函数（修改 ptr）
        delete ptr;
        ptr = nullptr;
    }
public:
    T& operator*() const noexcept {
        reset(); // ❌ 编译报错！const 成员函数不能调用非 const 成员函数
        return *ptr;
    }
};
```

| `const` 的位置              | 核心作用                                                     | 易错点纠正                                                   |
| --------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 成员函数后（`noexcept` 前） | 1. 函数内部不能修改类的普通成员变量（如智能指针的 `ptr`）；2. 只能被 `const` 类对象调用。 | 不约束返回值，也不约束返回的指针 / 引用能做什么。            |
| 返回值前（如 `const T*`）   | 1. 若返回 `const T*`：不能通过该指针修改**指向的对象**；2. 若返回 `const T&`：不能通过该引用修改**指向的对象**。 | 不限制 “指针变量本身的值”（比如可以让 `const T* p` 指向新地址），只限制 “指针指向的内容”。 |

### `operator->()`的链式调用规则

`operator->()`：运算符重载的固定语法，`operator` 是关键字，后面跟要重载的运算符 `*`，括号内无参数（因为是成员函数，操作数是对象本身）。

当你写 `sp->member` 时，编译器会先调用 `sp.operator->()` 得到原生指针 `ptr`，然后自动把代码拼接成 `ptr->member`（等价于 `(*ptr).member`）。

### 显式布尔转换运算符

这种`explicit operator bool()`是 C++ 中为自定义类实现的**显式布尔转换运算符**，也是布尔转换里非常重要的一个场景 —— 让自定义类的对象能像布尔值一样参与逻辑判断（比如`if (对象)`），同时避免隐式转换带来的坑。

在这里定义了用于判断对象是否 “有效 / 持有资源”；

```c++
unique_ptr<int> p = make_unique<int>(10);
if(p) // true 该智能指针持有资源
p.reset();
if(p) //false 该智能指针不持有资源
```

### 可变参数包的使用

```c++
template <typename T, typename... Args> // 声明类型参数包Args：代表任意数量/类型的集合
unique_ptr<T> make_unique(Args&&... args) // 声明形参参数包args：和Args一一对应，万能引用（Args&&）保证接收任意值类别
{
    return unique_ptr<T>(new T(std::forward<Args>(args)...)); // 使用args时必须加...展开
}
```

### `make_unique`为什么不能返回`unique_ptr<T>`的引用

函数内部创建的`unique_ptr<T>(...)`是**局部临时对象**（栈上创建），生命周期仅限于函数内部。

如果返回的是引用：

- 函数执行结束后，局部对象`temp`会被销毁（析构函数执行，释放资源）；
- 返回的引用会指向一个 “已经被销毁的对象”（悬垂引用）；
- 调用方使用这个引用时（比如`auto ptr = make_unique<Test>(10);`），访问的是内存中的垃圾数据，轻则程序崩溃，重则导致内存管理混乱（比如重复释放）。

返回值为`unique_ptr<T>`时，C++ 会触发**返回值优化（`RVO`）** 或**移动语义**：

1. **返回值优化（`RVO`）**：编译器会直接在调用方的内存空间创建`unique_ptr`对象，跳过临时对象的拷贝 / 移动，零开销；
2. **退而求其次：移动语义**：如果编译器未做 `RVO`，会将临时对象（将亡值）通过**移动构造函数**转移给调用方的对象，而非拷贝（ “移动赋值运算符” 的关联场景）。

简单说：返回值是 “传递对象的所有权”，返回引用是 “传递对象的别名”—— 而`make_unique`创建的是全新对象，只能传递所有权，不能传递别名（因为别名指向的对象会立刻销毁）。

## 实现简化`shared_ptr`

```c++
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

```

### `size_t`

`size_t` 并不是一个新的基础数据类型（比如 int、long），而是 C/C++ 标准库中通过 `typedef` 定义的**无符号整数类型别名**，它的本质是适配当前系统的无符号整数类型（比如 32 位系统下是 `unsigned int`，64 位系统下是 `unsigned long long`）。

- **无符号**：只能表示非负数（0 及正整数）；
- **大小适配系统**：长度等于当前系统的「指针宽度」（32 位系统占 4 字节，64 位系统占 8 字节），能完整表示当前系统中「对象的最大尺寸」。

设计 `size_t` 的核心目的是**统一「表示长度 / 大小 / 索引」的类型**，解决不同系统下整数类型长度不统一的问题。

# 跨平台线程池

## 核心概念

线程池（`ThreadPool`）是一种**线程管理模式**，它预先创建一组固定数量的工作线程，将待执行的任务放入任务队列中，工作线程循环从队列中取出任务并执行，避免了频繁创建 / 销毁线程的开销。线程池的核心要素包括：

- **工作线程**：预先创建的、可复用的线程，负责执行任务；
- **任务队列**：存储待执行的任务（通常是无返回值的可调用对象）；
- **同步机制**：通过互斥锁、条件变量实现任务队列的线程安全访问和线程唤醒；
- **启停控制**：通过原子变量控制线程池的启动 / 停止状态。

## 核心用途

- **降低线程开销**：线程的创建 / 销毁涉及内核态与用户态切换，线程池复用线程可大幅减少该开销；
- **控制并发数**：避免无限制创建线程导致的 CPU 上下文切换过载、内存耗尽；
- **任务异步调度**：将耗时任务提交到线程池，主线程无需阻塞等待，提升程序响应性；
- **资源管理**：统一管理线程生命周期，便于监控和控制并发任务的执行。

## 线程池项目的实现框架

该线程池基于 C++11 及以上标准实现，核心框架分为**私有成员**、**核心方法**、**公有接口**三部分，整体结构如下：

| 模块分类        | 核心组件                                                     | 作用说明                                                     |
| --------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 私有成员        | `std::vector<std::thread> threads`                           | 存储工作线程对象，管理线程生命周期                           |
|                 | `std::queue<std::function<void()>> task_queue`               | 任务队列，存储待执行的无参无返回值可调用对象                 |
|                 | `std::mutex mtx`                                             | 互斥锁，保护任务队列和停止标志的线程安全访问                 |
|                 | `std::condition_variable cv`                                 | 条件变量，实现工作线程的 “等待 - 唤醒” 机制，避免线程空轮询浪费 CPU |
|                 | `std::atomic<bool> stop{false}`                              | 原子布尔变量，标记线程池是否停止，原子操作避免竞态条件       |
| 核心私有方法    | `void NewThread()`                                           | 工作线程的核心循环逻辑：等待任务、取出任务、执行任务、处理停止逻辑 |
| 公有构造 / 析构 | `explicit ThreadPool(int ThreadNums)`                        | 构造函数：初始化指定数量的工作线程，绑定`NewThread`方法      |
|                 | `~ThreadPool() noexcept`                                     | 析构函数：安全停止线程池，唤醒所有线程，等待工作线程退出     |
| 禁用拷贝 / 移动 | 禁用拷贝构造 / 赋值，默认移动构造 / 赋值                     | 避免线程池对象拷贝导致的线程重复管理、资源冲突               |
| 公有接口        | `template<typename F,typename... Args> void submit(F&& f,Args&& ...args)` | 任务提交接口：将任意可调用对象（函数、lambda、绑定器）封装为任务入队 |

```c++
#include <iostream>
#include <mutex>
#include <chrono>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <string>
#include <vector>
#include <functional>

class ThreadPool
{

private:

    std::vector<std::thread> threads;
    std::queue<std::function<void()>> task_queue;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> stop{false};
    void NewThread()
    {
        while(true)
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock,[this](){ return !task_queue.empty() or stop.load();});
            if(stop.load() and task_queue.empty()) 
                break;
            if(task_queue.empty())
                continue;
            std::function<void()> task = std::move(task_queue.front());
            task_queue.pop();
            lock.unlock();
            task();
            task_count--; // 任务执行完之后再减少任务数
            if(task_count == 0)
            {
                // notify前需加锁，防止wait和notify间的竞态关系（还未阻塞就进行了notify）
                std::unique_lock<std::mutex> endlock(end_mtx);
                end_cv.notify_one();
            }
        }
    };  
    
public:

    std::mutex end_mtx;
    std::condition_variable end_cv;
    std::atomic<int> task_count{0};
    
    explicit ThreadPool(int ThreadNums) : stop(false)
    {
        for(int i=1;i<=ThreadNums;i++)
            threads.emplace_back(&ThreadPool::NewThread,this);
    };  

    ~ThreadPool() noexcept
    {
        {
            std::unique_lock<std::mutex> lock(mtx);
            stop.store(true);
        }   // 解锁（避免join时持有锁）
        cv.notify_all(); 
        for(auto &t : threads)
        {
            if(t.joinable())
                t.join();
        }
    }

    ThreadPool(const ThreadPool& other) = delete;
    ThreadPool& operator=(const ThreadPool& other) = delete;

    ThreadPool(ThreadPool&& other) noexcept = default;
    ThreadPool& operator=(ThreadPool&& other) noexcept = default;

    template<typename F,typename... Args>
    void submit(F&& f,Args&& ...args)
    {
        // 完美转发保证传入函数f的参数值类型不变
        std::function<void()> func = std::bind(std::forward<F>(f),std::forward<Args>(args)...);
        {
            std::unique_lock<std::mutex> lock(mtx);
            task_queue.emplace(std::move(func));
            task_count++;
        }
        // 离开作用域后，lock已自动解锁析构
        cv.notify_one();
    }
};
int main()
{
    ThreadPool pool(5);
    for(int i=1;i<=10;i++)
    {
        pool.submit([i]()
        {
            std::cout << "task " << i << " is running" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "task " << i << " is done" << std::endl;
        });
    }
    // 先让所有任务运行完，避免主程序提前退出析构线程池
    std::unique_lock<std::mutex> endlock(pool.end_mtx);
    pool.end_cv.wait(endlock,[&pool](){return pool.task_count == 0;});
    return 0;
}
```

## 核心 C++ 特性（按重要性排序）

| 特性名称                            | 代码中的应用场景                                             | 关键说明                                                     |
| ----------------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| 可变参数模板 + 完美转发             | `submit`方法的模板参数`F&& f, Args&& ...args` + `std::forward` | 1. 可变参数模板支持接收任意数量、任意类型的函数参数；2. 完美转发（`std::forward`）保证参数的左值 / 右值属性不变，避免不必要的拷贝；3. 适配任意签名的可调用对象（如带参数的函数、lambda）。 |
| 类型擦除（std::function）           | `task_queue`存储`std::function<void()>`，`submit`将任意可调用对象封装为该类型 | 1. `std::function<void()>`擦除具体可调用对象的类型，统一任务队列的存储类型；2. 支持任意签名的可调用对象（通过`std::bind`绑定参数后转为无参）。 |
| 智能锁（std::unique_lock）          | 条件变量等待、任务入队 / 出队时使用`std::unique_lock<std::mutex>` | 1. `unique_lock`支持手动解锁（执行任务前解锁，避免锁持有时间过长）；2. 自动析构解锁，避免手动解锁遗漏导致的死锁；3. 配合条件变量的`wait`方法实现锁的临时释放。 |
| 条件变量（std::condition_variable） | `cv.wait(lock, 条件)` + `cv.notify_one()`/`cv.notify_all()`  | 1. `wait`：释放锁并阻塞，直到条件满足（有任务 / 线程池停止），重新获取锁；2. `notify_one`：提交任务后唤醒一个等待的工作线程；3. `notify_all`：析构时唤醒所有线程，确保线程退出。 |
| 原子类型（std::atomic）             | `std::atomic<bool> stop`                                     | 1. 原子操作无需加锁，保证多线程下`stop`的读写一致性；2. 避免普通布尔变量因编译器优化导致的 “可见性问题”（如线程读取到过期的`stop`值）。 |
| Lambda 表达式                       | 构造函数绑定线程、`submit`提交 lambda 任务、条件变量的谓词判断 | 1. 简化匿名函数的定义，无需显式定义函数对象；2. 捕获外部变量（如`i`）实现任务参数传递；3. 条件变量的谓词（`[this](){ ... }`）简化条件判断逻辑。 |
| 右值引用 + 移动语义                 | `std::move(task_queue.front())`、`task_queue.emplace(std::move(func))` | 1. 移动语义避免`std::function`等大对象的拷贝开销；2. `emplace`直接在队列中构造对象，减少临时对象创建。 |
| 线程库（std::thread）               | `threads.emplace_back(&ThreadPool::NewThread,this)`          | 1. `emplace_back`直接构造线程对象，避免拷贝；2. 绑定类成员函数时传递`this`指针，让工作线程访问类的成员变量。 |
| 显式构造函数 + 禁用拷贝             | `explicit ThreadPool(int ThreadNums)`、删除拷贝构造 / 赋值   | 1. `explicit`避免隐式类型转换（如`ThreadPool pool = 5`）；2. 禁用拷贝防止线程池对象拷贝导致的线程重复 join、资源冲突。 |

## 核心设计思想

### “生产者 - 消费者” 模型

线程池的核心是经典的生产者 - 消费者模型：

- **生产者**：调用`submit`的线程（如主线程）向任务队列添加任务；
- **消费者**：工作线程从任务队列取出任务并执行；
- 同步机制：互斥锁保护队列访问，条件变量实现 “无任务时等待，有任务时唤醒”，避免空轮询。

### 懒加载与资源复用

- 工作线程在构造时创建，直到线程池析构才退出，复用线程执行多个任务，避免频繁创建 / 销毁线程的开销；
- 任务执行前解锁互斥锁（`lock.unlock()`），仅在操作任务队列时加锁，最小化锁持有时间，提升并发效率。

### 安全停止机制

析构函数的停止逻辑遵循 “安全退出” 原则：

1. 设置`stop = true`（原子操作）；
2. 调用`cv.notify_all()`唤醒所有等待的工作线程；
3. 遍历所有工作线程，调用`join()`等待线程退出；
4. 仅当`stop`为`true`且任务队列为空时，工作线程才退出循环，保证已入队的任务全部执行完毕。

### 泛型任务封装

通过`std::bind` + `std::function` + 可变参数模板，实现对任意可调用对象的封装：

- `std::bind(std::forward<F>(f), std::forward<Args>(args)...)`：将带参数的可调用对象绑定为无参函数；
- `std::function<void()>`：统一任务类型，适配 lambda、普通函数、成员函数等不同可调用对象。

### 避免竞态条件

- 互斥锁保护任务队列的`push/pop/empty`操作，避免多线程同时修改队列；
- 原子变量`stop`避免 “读 - 改 - 写” 竞态；
- 条件变量的谓词判断（`!task_queue.empty() or stop.load()`）避免 “虚假唤醒”（spurious wakeup）。

## 工作线程核心循环（NewThread 方法）

```cpp
void NewThread() 
{
    while(true) 
    {
        std::unique_lock<std::mutex> lock(mtx);
        // 线程阻塞等待：仅当「任务队列非空（有任务可执行）」或「线程池停止（需要退出）」时，才会退出阻塞
        // 若条件不满足，线程释放锁并阻塞；被唤醒后会重新检查条件，避免虚假唤醒
        cv.wait(lock,[this](){ return !task_queue.empty() or stop.load();});
        // 停止条件：线程池停止 且 任务队列为空 → 退出循环
        if(stop.load() and task_queue.empty()) 
            break;
        // 防御性判断：避免虚假唤醒导致的空队列访问
        if(task_queue.empty())
            continue;
        // 取出任务并移动，减少拷贝
        std::function<void()> task = std::move(task_queue.front());
        task_queue.pop();
        // 执行任务前解锁，避免锁持有时间过长
        lock.unlock();
        // 执行任务
        task();
        task_count--;
        if(task_count == 0)
        {
            std::unique_lock<std::mutex> endlock(end_mtx);
            end_cv.notify_one();
		}
    }
}
```

- **wait 的谓词**：必须用谓词判断而非单纯`wait`，因为条件变量可能被虚假唤醒（即使没有 notify），谓词确保只有 “有任务” 或 “线程池停止” 时才继续；
- **解锁时机**：执行任务前解锁，避免任务执行期间持有锁，导致其他线程无法提交任务 / 取出任务，最大化并发效率；
- **停止逻辑**：仅当 “停止标志为 true + 任务队列为空” 时退出，保证所有已入队任务执行完毕后线程才退出。

## 任务提交接口（submit 方法）

```cpp
template<typename F,typename... Args>
void submit(F&& f,Args&& ...args) {
    // 封装为无参可调用对象
    std::function<void()> func = std::bind(std::forward<F>(f),std::forward<Args>(args)...);
    {
        std::unique_lock<std::mutex> lock(mtx);
        // 移动入队，避免拷贝
        task_queue.emplace(std::move(func));
        task_count++;
    } // 作用域结束，lock自动解锁
    // 唤醒一个等待的工作线程
    cv.notify_one();
}
```

- **完美转发**：`std::forward<F>(f)`和`std::forward<Args>(args)`保证传入的函数和参数的左值 / 右值属性不变，例如：
  - 传入右值 lambda 时，直接移动而非拷贝；
  - 传入左值参数时，按引用传递（需确保参数生命周期长于任务执行周期）；
- **bind 的作用**：将任意签名的可调用对象（如`void f(int, std::string)`）绑定参数后，转为`std::function<void()>`类型，适配任务队列；
- **notify_one**：提交一个任务仅唤醒一个线程，避免 “惊群效应”（多个线程被唤醒但只有一个能取到任务）。
- **任务完成通知（end_cv）**：必须加锁 → 防止信号丢失，主线程永久阻塞；
- **任务队列唤醒（cv）**：无需加锁 → 有谓词兜底，信号丢失无影响；

## 析构函数的安全停止

```cpp
~ThreadPool() noexcept {
    std::unique_lock<std::mutex> lock(mtx);
    stop.store(true);
    cv.notify_all(); 
    lock.unlock(); // 提前解锁，避免join时持有锁
    for(auto &t : threads) {
        if(t.joinable())
            t.join();
    }
}
```

- **notify_all**：必须唤醒所有等待的线程，否则部分线程可能卡在`cv.wait`中无法退出；
- **joinable 判断**：避免对已 join 的线程重复 join 导致未定义行为；
- **提前解锁**：`join`会阻塞主线程，若持有锁会导致其他线程（如最后执行任务的线程）无法访问锁，提前解锁避免死锁。

## 原子变量 stop 的必要性

`std::atomic<bool> stop`替代普通`bool`的核心原因：

- 普通`bool`的读写可能被编译器优化为 “缓存值”，导致线程无法及时感知`stop`的变化；
- 原子操作无需加锁，保证多线程下`stop`的读写是 “原子的、可见的”，例如：
  - 主线程设置`stop = true`后，所有工作线程能立即读取到最新值；
  - 避免 “工作线程读取到过期的 stop 值，一直等待任务” 的问题。

## Thread构造参数（emplace_back内写法）

| 方法           | 传参要求                                 | 底层行为                                                     |
| -------------- | ---------------------------------------- | ------------------------------------------------------------ |
| `push_back`    | 传入**已构造好的对象实例**（或临时对象） | 1. 先创建 / 拿到对象实例；2. 将对象**拷贝 / 移动**到 vector 尾部的内存中； |
| `emplace_back` | 传入**对象的构造参数**                   | 1. 直接在 vector 尾部的内存空间里；2. 用传入的参数**就地构造**对象； |

`emplace_back` 的核心是**完美转发（perfect forwarding）** —— 你传入的参数会原封不动地传给目标对象的构造函数，因此：

- 只要是目标对象构造函数支持的参数组合，都能直接传给 `emplace_back`；
- 无需手动构造对象，由容器帮你完成 “就地构造”。

- `push_back` 是 “先有对象，再塞进去”（拷贝 / 移动）；
- `emplace_back` 是 “告诉容器怎么造对象，容器自己在原地造”（传构造参数，就地构造）。

而代码中 `threads.emplace_back(&ThreadPool::NewThread, this)` 的写法，正是利用了 `emplace_back` 的特性：把 `std::thread` 构造需要的参数（成员函数指针 + this）直接传给它，让容器在自己的内存里造线程对象，既简洁又高效。

**`std::thread` 如何执行类成员函数？**

`ThreadPool::NewThread` 是**类的非静态成员函数**，这类函数有一个隐藏特性：

✅ 非静态成员函数的第一个参数是隐含的`this`指针（指向类实例），必须显式传递才能调用。

如果直接写：

```cpp
// 错误：NewThread是成员函数，缺少this指针，无法绑定到线程
threads.emplace_back(&ThreadPool::NewThread); 
```

`std::thread`的构造函数要求：**若传入的是类成员函数，必须紧跟该类的实例指针（this）作为第二个参数**，格式为：

```cpp
std::thread(成员函数指针, 类实例指针, 函数参数...);
```

对应到你的代码：

- `&ThreadPool::NewThread`：成员函数指针（指向`NewThread`的地址）；
- `this`：当前`ThreadPool`实例的指针（给`NewThread`传递隐含的`this`）；
- 无额外参数：`NewThread`是无参函数，因此无需后续参数。

```cpp
// 伪代码：std::thread 构造函数的逻辑
template <typename Callable, typename... Args>
thread(Callable&& func, Args&&... args) {
    // 1. 保存可调用对象（func）和参数（args）
    // 2. 启动线程，执行：func(args...)
}
```