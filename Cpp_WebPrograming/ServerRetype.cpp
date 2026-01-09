#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

class TCPServer 
{
private:
    int server_fd;
    int client_fd;
    const uint16_t port;

    void error(const std::string& msg, bool CloseServer = true)
    {
        std::cerr << "[Error] " << msg << std::endl;
        if(CloseServer and server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        if(client_fd != -1)
        {
            close(client_fd);
            client_fd = -1;
        }
        throw std::runtime_error(msg);
    }

public:
    // 构造函数：初始化端口，初始化套接字为无效值
    explicit TCPServer(const uint16_t _port) : port(_port), server_fd(-1), client_fd(-1) {}

    ~TCPServer() noexcept
    {
        if(server_fd != -1) close(server_fd);
        if(client_fd != -1) close(client_fd);
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
        server_addr.sin_port = htons(port); // 使用构造函数初始化后的端口
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
    // 等待并接受客户端的连接
    void AcceptClient()
    {
        sockaddr_in client_addr{};
        socklen_t siz = sizeof(client_addr);
        // 阻塞并等待客户端连接
        client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &siz);
        if(client_fd == -1)
        {
            error("Failed to accept client connection!");
        }
        // 打印客户端信息
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip));
        int __port = ntohs(client_addr.sin_port);
        std::cout << "Client connected: IP = " << client_ip << ", Port = " << __port << std::endl;
    }

    void communicate()
    {
        char buffer[1024]{};
        while(true)
        {
            ssize_t msg_len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if(msg_len > 0)
            {
                buffer[msg_len] = '\0';
                std::cout << "Client message : " << buffer << std::endl;

                // 发送响应给客户端
                std::string response = "Message received : " + std::string(buffer);
                // 注意此时是response.size() 而不是 sizeof(response) ! 这两者完全不同
                ssize_t respon_len = send(client_fd, response.c_str(), response.size(), 0);
                if(respon_len == -1)
                {
                    error("Failed to send response to client!");
                }
            }
            else if(msg_len == 0)
            {
                std::cout << "[Info] Client disconnected!" << std::endl;
                break;
            }
            else
            {
                // 仅退出通信，而不关闭服务端
                error("Failed to receive data from client!", false);
                break; 
            }
        }
    }
};

int main()
{
    TCPServer server(9999);
    server.init();
    server.AcceptClient();
    server.communicate();

    std::cout << "The server exited successfully!" << std::endl;
    return EXIT_SUCCESS;
}
