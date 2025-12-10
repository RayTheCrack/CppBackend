#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>
// 策略接口（排序算法）
class SortStrategy {
public:
    virtual void sort(std::vector<int>& vec) = 0;
    virtual ~SortStrategy() = default;
};
// 具体策略1：冒泡排序
class BubbleSort : public SortStrategy {
public:
    void sort(std::vector<int>& vec) override {
        std::cout << "使用冒泡排序" << std::endl;
        int n = vec.size();
        for (int i = 0; i < n-1; ++i) {
            for (int j = 0; j < n-i-1; ++j) {
                if (vec[j] > vec[j+1]) {
                    std::swap(vec[j], vec[j+1]);
                }
            }
        }
    }
};
// 具体策略2：快速排序
class QuickSort : public SortStrategy {
public:
    void sort(std::vector<int>& vec) override {
        std::cout << "使用快速排序" << std::endl;
        std::sort(vec.begin(), vec.end()); // 复用标准库快速排序
    }
};
// 上下文类（使用策略的类）
class Sorter 
{
private:
    std::unique_ptr<SortStrategy> strategy; // 独占智能指针管理策略对象
public:
    // 构造函数：接收右值引用的unique_ptr，移动语义接管所有权
    Sorter(std::unique_ptr<SortStrategy>&& s) : strategy(std::move(s)) {}
    // 析构函数：无需手动释放，unique_ptr自动处理
    ~Sorter() = default;
    // 设置新策略：接收unique_ptr（右值引用），移动语义替换旧策略
    void setStrategy(std::unique_ptr<SortStrategy>&& s) {
        strategy = std::move(s); // 旧策略会被unique_ptr自动释放
    }
    // 执行排序
    void sortData(std::vector<int>& vec) {
        if (!strategy) { // 空指针保护，避免访问空策略
            std::cerr << "错误：未设置排序策略！" << std::endl;
            return;
        }
        strategy->sort(vec);
        // 打印结果
        for (int num : vec) {
            std::cout << num << " ";
        }
        std::cout << std::endl;
    }
};
// 测试
int main() 
{
    std::vector<int> data1 = {5, 2, 9, 1, 5, 6}; // 冒泡排序用
    std::vector<int> data2 = {5, 2, 9, 1, 5, 6}; // 快速排序用（避免重复排序同一数组）
    // 使用冒泡排序：用make_unique创建策略对象，移动到Sorter
    Sorter sorter(std::make_unique<BubbleSort>());
    sorter.sortData(data1);
    // 切换为快速排序：同样用make_unique创建，移动替换旧策略
    sorter.setStrategy(std::make_unique<QuickSort>());
    sorter.sortData(data2);
    return 0;
}