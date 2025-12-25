#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sys/wait.h>


int main()
{
    pid_t pid = fork();
    if(pid == -1)
    {
        std::cerr << "Fork failed!" << std::endl;
        return 1;
    }
    else if(pid > 0)
    {
        // Parent process
        std::cout << "This is the parent process. Parent Process ID: " << getpid() << std::endl;
        int status;
        pid_t waited_pid = waitpid(pid, &status, 0);
        if(waited_pid == -1)
        {
            std::cerr << "waitpid failed!" << std::endl;
            return 1;
        }
        if(WIFEXITED(status))
        {
            std::cout << "Child process " << waited_pid << " exited with status " << WEXITSTATUS(status) << std::endl;
        }
    }
    else
    {
        // Child process
        std::cout << "This is the child process. Child Process ID: " << getpid() << " Parent Process ID: " << getppid() << std::endl;
        // Simulate some work in child process
        sleep(2);
        std::cout << "Child process " << getpid() << " is exiting." << std::endl;
        return 42; // Exit with a specific status
    }


    return 0;
}