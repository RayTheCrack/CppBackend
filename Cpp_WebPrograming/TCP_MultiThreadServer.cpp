#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <thread> 

class TCPServer 
{
private:
    int server_fd;
    const uint16_t port;  // 移除client_fd，每个线程独立管理客户端fd

    // 错误处理函数（调整：不再操作client_fd，仅处理server_fd）
    void error(const std::string& msg, bool CloseServer = true)
    {
        std::cerr << "[Error] " << msg << std::endl;
        if(CloseServer and server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        exit(EXIT_FAILURE);
    }

    // 客户端通信工作函数
    void client_communicate(int client_fd, const std::string& client_ip, uint16_t client_port)
    {
        char buffer[1024]{};
        while(true)
        {
            ssize_t msg_len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if(msg_len > 0)
            {
                buffer[msg_len] = '\0';
                std::cout << "Client [" << client_ip << ":" << client_port << "] message : " << buffer << std::endl;

                // 发送响应给客户端
                std::string response = "Message received : " + std::string(buffer);
                ssize_t respon_len = send(client_fd, response.c_str(), response.size(), 0);
                if(respon_len == -1)
                {
                    std::cerr << "[Error] Failed to send response to client [" << client_ip << ":" << client_port << "]" << std::endl;
                    break;
                }
            }
            else if(msg_len == 0)
            {
                std::cout << "[Info] Client [" << client_ip << ":" << client_port << "] disconnected!" << std::endl;
                break;
            }
            else
            {
                std::cerr << "[Error] Failed to receive data from client [" << client_ip << ":" << client_port << "]" << std::endl;
                break; 
            }
        }
        // 通信结束后关闭当前客户端套接字
        if(client_fd != -1)
        {
            close(client_fd);
            client_fd = -1;
        }
    }

public:

    // 构造函数：初始化端口，初始化套接字为无效值
    explicit TCPServer(const uint16_t _port) : port(_port), server_fd(-1) {}

    ~TCPServer() noexcept
    {
        if(server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        std::cout << "[Info] TCPServer destroyed!" << std::endl;
    }

    // 初始化服务器，创建套接字->绑定->监听
    void init()
    {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(server_fd == -1)
        {
            error("Failed to create a socket!");
        }

        sockaddr_in server_addr{};  
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        int ret = bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
        if(ret == -1)
        {
            error("Failed to bind socket!");
        }

        int lis = listen(server_fd, 128);
        if(lis == -1)
        {
            error("Failed to set listen socket!");
        }
        std::cout << "Server initialized successful! Listening on port: " << port << std::endl;
    }

    // 循环接受客户端连接，并为每个客户端创建独立线程
    void accept_clients_loop()
    {
        std::cout << "[Info] Server is waiting for client connections..." << std::endl;
        while(true)  // 持续接受客户端连接
        {
            sockaddr_in client_addr{};
            socklen_t client_addr_len = sizeof(client_addr);
            // 阻塞等待客户端连接
            int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);
            if(client_fd == -1)
            {
                std::cerr << "[Error] Failed to accept client connection! Continue waiting..." << std::endl;
                continue;
            }

            // 获取客户端IP和端口
            char client_ip[INET_ADDRSTRLEN] = {0};
            inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip));
            uint16_t client_port = ntohs(client_addr.sin_port);

            std::cout << "[Info] New client connected: IP = " << client_ip << ", Port = " << client_port << std::endl;

            // 为当前客户端创建独立线程，执行通信逻辑
            std::thread client_thread(&TCPServer::client_communicate, this, client_fd, std::string(client_ip), client_port);
            // 分离线程：线程执行完毕后自动释放资源，不阻塞主线程
            client_thread.detach();
        }
    }
};

int main()
{
    TCPServer server(9999);
    server.init();
    server.accept_clients_loop();  // 进入循环接受客户端连接
    std::cout << "The server exited successfully!" << std::endl;
    return EXIT_SUCCESS;
}