#include <sys/types.h>
#include <iostream>
#include <unistd.h>

int main()
{
    pid_t pid = getpid();
    std::cout << "Current Process ID: " << pid << std::endl;
    pid_t ppid = getppid();
    std::cout << "Parent Process ID: " << ppid << std::endl;
    pid_t new_pid = fork();
    if(new_pid < 0)
    {
        std::cerr << "Fork failed!" << std::endl;
        return 1;
    }
    else
    {
        if(new_pid == 0)
        {
            std::cout << "This is the child process. Child Process ID: " << getpid() << " Parents Process ID : " << getppid() << std::endl;
        }
        else
        {
            std::cout << "This is the parent process. Parent Process ID: " << getpid() << std::endl;
        }
    }
    return 0;
}