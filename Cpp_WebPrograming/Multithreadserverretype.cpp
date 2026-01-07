#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>

class TCPServer
{
private:
    int server_fd;
    const uint16_t server_port;

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
    // 多线程客户端通信函数
    void client_communicate(int client_fd, const std::string& client_ip, uint16_t client_port)
    {
        char buffer[1024] = {0};
        while(true)
        {
            ssize_t msg_len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if(msg_len > 0)
            {
                buffer[msg_len] = '\0';
                std::cout << "Client[" << client_ip << ":" << client_port << "] message: " << buffer << std::endl;

                std::string response = "Message received : " + std::string(buffer);
                ssize_t resp_len = send(client_fd, response.c_str(), response.size(), 0);
                if(resp_len == -1)
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
        // 通信结束后，关闭套接字
        if(client_fd != -1)
        {
            close(client_fd);
            client_fd = -1;
        }
    }

public:
    explicit TCPServer(const uint16_t _port) : server_port(_port), server_fd(-1) {}

    ~TCPServer() noexcept
    {
        if(server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        std::cout << "[Info] TCPServer destoryed!" << std::endl;
    }

    void init()
    {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(server_fd == -1)
        {
            error("Failed to create server socket!");
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        // 优化：添加套接字复用选项，允许端口快速复用
        int opt = 1;
        if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1)
        {
            error("Failed to set socket options!");
        }

        if(bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1)
        {
            error("Failed to bind socket!");
        }

        int lis = listen(server_fd, 128);
        if(lis == -1)
        {
            error("Failed to listen server socket!");
        }
        std::cout << "Successfully initialized the server! Listening on port : " << server_port << std::endl;
    }

    void Accept_Client_Loop()
    {
        std::cout << "[Info] Server is waiting for client connections..." << std::endl;
        while(true)
        {
            sockaddr_in client_addr{};
            socklen_t addrlen = sizeof(client_addr);
            // 阻塞并等待客户端的连接
            int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &addrlen);
            if(client_fd == -1)
            {
                std::cerr << "[Error] Failed to accept client connection! Continue waiting..." << std::endl;
                continue;
            }
            // 获取客户端的ip和端口
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
    server.Accept_Client_Loop();
    std::cout << "The server exited successfully!" << std::endl;
    return EXIT_SUCCESS;
}