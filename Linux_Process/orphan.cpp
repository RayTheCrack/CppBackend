#include <sys/types.h>
#include <sys/stat.h>  
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

int main()
{
    pid_t pid = fork();
    if(pid > 0)
    {
        // Parent process
        std::cout << "This is the parent process. Parent Process ID: " << getpid() << std::endl;
    }
    else if(pid == 0)
    {
        // Child process
        std::cout << "This is the child process. Child Process ID: " << getpid() << " Parent Process ID: " << getppid() << std::endl;
        // Simulate orphaning by sleeping to allow parent to exit
        sleep(2);
        std::cout << "Orphaned Child Process ID: " << getpid() << " New Parent Process ID: " << getppid() << std::endl;
    }
    return 0;
}