#include <unistd.h>
#include <iostream>
#include <string>
#include <arpa/inet.h>

int main()
{
    // 1. 创建监听的套接字
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1)
    {
        perror("Failed to create a socket!");
        close(server_fd);
        return -1;
    }
    // 2. 绑定本地的IP Port
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    // 记得将字节序转为大端，即主机字节序转为网络字节序，端口为short类型
    saddr.sin_port = htons(9999);
    // 绑定到本地IP地址，INADDR_ANY表示绑定到所有可用的网络接口
    saddr.sin_addr.s_addr = INADDR_ANY;
    int ret = bind(server_fd, (sockaddr*)&saddr, sizeof(saddr));
    if(ret == -1)
    {
        perror("Failed to bind the socket!");
        close(server_fd);
        return -1;
    }
    // 3. 监听套接字
    ret = listen(server_fd, 128);
    if(ret == -1)
    {
        perror("Failed to listen on the socket!");
        close(server_fd);
        return -1;
    }
    //4. 阻塞并等待客户端连接
    sockaddr_in caddr;
    socklen_t caddr_len = sizeof(caddr);
    int client_fd = accept(server_fd, (sockaddr*)&caddr, &caddr_len);
    if(client_fd == -1)
    {
        perror("Failed to accept a client connection!");
        close(server_fd);
        return -1;
    }
    // 连接成功，打印客户端的IP和Port
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &caddr.sin_addr.s_addr, client_ip, sizeof(client_ip));
    uint16_t client_port = ntohs(caddr.sin_port);
    std::cout << "Client connected from IP: " << client_ip << ", Port: " << client_port << std::endl;
    // 5. 通信
    while(1)
    {
        // 接收数据
        char buffer[1024] = {0};
        int len = recv(client_fd, buffer, sizeof(buffer), 0);
        if(len > 0)
        {
            std::cout << "Client says: " << buffer << std::endl;
            // 发送数据
            std::string response = "Message received: " + std::string(buffer);
            send(client_fd, response.c_str(), response.size(), 0); 
        }
        else if(len == 0)
        {
            std::cout << "Client disconnected." << std::endl;
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
    close(server_fd);

    return 0;
}