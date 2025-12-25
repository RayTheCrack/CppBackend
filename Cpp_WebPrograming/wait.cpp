#include <iostream>
#include <cstdlib>   // for exit()
#include <unistd.h>  // for fork(), getpid(), sleep()
#include <sys/wait.h> // for wait()
#include <sys/types.h> // for pid_t

using namespace std;

int main() {
    // 创建子进程
    pid_t pid = fork();
    if(pid == -1) 
    {
        // fork 失败
        perror("fork failed");
        exit(EXIT_FAILURE);
    } 
    else if (pid == 0) 
    {
        // 子进程逻辑
        cout << "子进程（PID：" << getpid() << "）正在运行，即将退出..." << endl;
        sleep(2); // 模拟子进程执行任务
        cout << "子进程（PID：" << getpid() << "）已退出" << endl;
        exit(EXIT_SUCCESS); // 子进程正常退出
    } 
    else 
    {
        // 父进程逻辑
        cout << "父进程（PID：" << getpid() << "）等待子进程（PID：" << pid << "）回收..." << endl;
        // 阻塞回收任意子进程，不获取退出状态（传入NULL）
        pid_t recycled_pid = wait(NULL);
        if(recycled_pid == -1) 
        {
            perror("wait failed");
            exit(EXIT_FAILURE);
        }

        cout << "父进程成功回收子进程（PID：" << recycled_pid << "），无僵尸进程" << endl;
        sleep(5); // 父进程继续运行，可通过 ps -ef | grep 程序名 验证无僵尸进程
    }

    return 0;
}