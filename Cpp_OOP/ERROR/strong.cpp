#include <iostream>
#include <vector>
#include <stdexcept>
#include <algorithm> // swap
using namespace std;
class MyData {
private:
    vector<int> nums = {1,2,3};
public:
    // 强保证：修改前先拷贝，异常不影响原数据
    void modify() {
        // 步骤1：拷贝原数据到临时对象（副本）
        vector<int> temp = nums;
        // 步骤2：在副本上修改（抛异常也不影响原数据）
        temp[0] = 100;
        throw runtime_error("测试异常"); 
        temp[1] = 200;
        // 步骤3：交换副本和原数据（swap 是 no-throw 操作）
        swap(nums, temp);
    }
    void print() {
        for (int num : nums) cout << num << " ";
        cout << endl;
    }
};
int main() {
    MyData data;
    try {
        data.modify();
    } catch (const exception& e) {
        cout << e.what() << endl;
    }
    // 异常后，原数据完全没变化（{1,2,3}）→ 强保证
    data.print();
    return 0;
}