#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <unistd.h>
#include <stdexcept>
#include <atomic>
#include <cstring>
class UDPServer
{
private:
    const uint16_t server_port;
    int server_fd;
    std::atomic<bool> is_running;

    void error(const std::string& msg,bool CloseServer = true)
    {
        std::cerr << "[Error] " << msg << std::endl;
        if(CloseServer and server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        throw std::runtime_error(msg);
    }

public:

    explicit UDPServer(const uint16_t _port) : server_port(_port), server_fd(-1), is_running(true) {}
    
    UDPServer(const UDPServer& other) = delete;
    UDPServer& operator=(const UDPServer& other) = delete;

    UDPServer(UDPServer&& other) = default;
    UDPServer& operator=(UDPServer&& other) = default;
    
    ~UDPServer() noexcept
    {
        if(server_fd != -1) close(server_fd);
        std::cout << "UDPServer destoryed!" << std::endl;
    }

    void init()
    {
        server_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if(server_fd == -1)
        {
            error("Failed to create a socket!");
        }

        int opt = 1;
        if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1)
        {
            error("Failed to set socket options!");
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        int ret = bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
        if(ret == -1)
        {
            error("Failed to bind socket!");
        }

        std::cout << "[Info] UDP Server initialized successfully! Listening on port: " << server_port << std::endl;
    }

    void send_msg(const std::string& msg, const sockaddr_in& client_addr)
    {       
        ssize_t send_len = sendto(
            server_fd,
            msg.c_str(),
            msg.size(),
            0,
            reinterpret_cast<const sockaddr*>(&client_addr),
            sizeof(client_addr)
        );
        if(send_len == -1)
        {
            std::cerr << "[Warning] Failed to send message to client " 
                      << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port);
        }
        else
        {
            std::cout << "[Info] Sent response to client: " << msg << std::endl;
        }
    }

    void start_communicate()
    {

        char buffer[1024] = {0}; // 接收缓冲区
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);

        std::cout << "[Info] UDP Server started! Waiting for client messages..." << std::endl;

        while (is_running.load())
        {
            // 接收客户端消息（UDP无连接，每次recvfrom获取客户端地址）
            ssize_t recv_len = recvfrom(
                server_fd,
                buffer,
                sizeof(buffer) - 1, // 留1字节给'\0'
                0,
                reinterpret_cast<sockaddr*>(&client_addr),
                &client_addr_len
            );

            // 处理接收中断（如SIGINT信号）
            if (recv_len == -1)
            {
                if (errno == EINTR and is_running.load())
                {
                    memset(buffer, 0, sizeof(buffer));
                    continue;
                }
                else
                {
                    error("Failed to receive message!", false);
                    break;
                }
            }

            // 处理空消息
            if (recv_len == 0)
            {
                continue;
            }

            // 解析客户端信息
            buffer[recv_len] = '\0'; // 字符串结束符
            std::string client_ip = inet_ntoa(client_addr.sin_addr);
            uint16_t client_port = ntohs(client_addr.sin_port);
            std::string recv_msg = buffer;

            // 打印客户端消息
            std::cout << "[Client " << client_ip << ":" << client_port << "] Message: " << recv_msg << std::endl;

            // 构造响应消息并发送
            std::string response = "Server received your message: " + recv_msg;
            send_msg(response, client_addr);

            // 清空缓冲区
            memset(buffer, 0, sizeof(buffer));
        }

        std::cout << "[Info] UDP Server stopped receiving messages." << std::endl;

    };

    void stop()
    {
        is_running.store(false);
        if(server_fd != -1) shutdown(server_fd, SHUT_RD);
    }

};

int main()
{
    try
    {
        // 创建UDP服务器（监听9999端口）
        UDPServer server(9999);
        // 初始化并启动服务器
        server.init();
        server.start_communicate();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "[Fatal] Server error: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "[Info] UDP Server exited normally." << std::endl;
    return 0;
}