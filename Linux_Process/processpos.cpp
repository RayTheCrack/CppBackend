#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

int main()
{
    pid_t pid = fork();
    if(pid < 0)
    {
        std::cerr << "Fork failed!" << std::endl;
        return 1;
    }
    else
    {
        if(pid == 0)
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