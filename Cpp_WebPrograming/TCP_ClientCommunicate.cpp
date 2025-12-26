#include <unistd.h>
#include <iostream>
#include <string>
#include <arpa/inet.h>

int main()
{
    // 1. 创建通信的套接字
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd == -1)
    {
        perror("Failed to create a socket!");
        close(client_fd);
        return -1;
    }
    // 2. 绑定服务器的IP Port
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    // 记得将字节序转为大端，即主机字节序转为网络字节序，端口为short类型
    saddr.sin_port = htons(9999);
    // 绑定到服务器IP地址
    inet_pton(AF_INET, "192.168.182.128", &saddr.sin_addr.s_addr);
    // 3. 连接服务器
    int ret = connect(client_fd, (sockaddr*)&saddr, sizeof(saddr));
    if(ret == -1)
    {
        perror("Failed to bind the socket!");
        close(client_fd);
        return -1;
    }
    // 4. 通信
    while(1)
    {
        // 发送数据
        std::string send_msg;
        std::cout << "Enter message to send: ";
        std::getline(std::cin, send_msg);
        int send_len = send(client_fd, send_msg.c_str(), send_msg.size(), 0);
        if(send_len == -1)
        {
            perror("Failed to send data!");
            break;
        }
        // 接收数据
        char buffer[1024] = {0};
        int len = recv(client_fd, buffer, sizeof(buffer), 0);
        if(len > 0)
        {
            std::string recv_msg(buffer, len);
            std::cout << "Received from server: " << recv_msg << std::endl;
        }
        else if(len == 0)
        {
            std::cout << "Server closed the connection." << std::endl;
            break;
        }
        else
        {
            perror("Failed to receive data!");
            break;
        }
    }

    // 6. 关闭套接字
    close(client_fd);

    return 0;
}