#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
using namespace std;

int main()
{
    for(int i=1;i<=3;i++)
    {
        pid_t pid = fork();
        if(pid == 0) break;
        cout << "created process " << i << " with PID: " << pid << endl;
    }

    return 0;
}