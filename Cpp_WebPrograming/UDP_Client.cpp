#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <cerrno>

class UDPClient
{
private:
    const std::string server_ip;
    const uint16_t server_port;
    int client_fd;

    void error(const std::string& msg, bool close_client = true)
    {
        std::cerr << "[Error] " << msg << " (errno: " << errno << ")" << std::endl;
        if (close_client && client_fd != -1)
        {
            close(client_fd);
            client_fd = -1;
        }
        throw std::runtime_error(msg);
    }

public:

    UDPClient(const std::string& _ip, const uint16_t _port) 
        : server_ip(_ip), server_port(_port), client_fd(-1) {}

    // 禁用拷贝，允许移动
    UDPClient(const UDPClient&) = delete;
    UDPClient& operator=(const UDPClient&) = delete;
    UDPClient(UDPClient&&) = default;
    UDPClient& operator=(UDPClient&&) = default;

    // 析构函数：关闭套接字
    ~UDPClient() noexcept
    {
        if (client_fd != -1)
        {
            close(client_fd);
        }
        std::cout << "[Info] UDPClient destroyed!" << std::endl;
    }

    void init()
    {
        client_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if(client_fd == -1)
        {
            error("Failed to create client socket!");
        }
        std::cout << "[Info] UDP Client initialized successfully! Target server: " 
                  << server_ip << ":" << server_port << std::endl;
    }

    // 发送消息到服务器，并接收响应
    void send_and_receive(const std::string& msg)
    {
        if (client_fd == -1)
        {
            error("Client socket not initialized!", false);
        }

        // 构造服务器地址结构体
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        
        // 转换IP地址格式
        if(inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0)
        {
            error("Invalid server IP address!");
        }

        // 发送消息到服务器
        ssize_t send_len = sendto(
            client_fd,
            msg.c_str(),
            msg.size(),
            0,
            reinterpret_cast<const sockaddr*>(&server_addr),
            sizeof(server_addr)
        );

        if (send_len == -1)
        {
            error("Failed to send message to server!");
        }
        std::cout << "[Info] Sent message to server: " << msg << std::endl;

        char buffer[1024] = {0};
        socklen_t server_addr_len = sizeof(server_addr);
        ssize_t recv_len = recvfrom(
            client_fd,
            buffer,
            sizeof(buffer) - 1, // 留1字节给结束符
            0,
            reinterpret_cast<sockaddr*>(&server_addr),
            &server_addr_len
        );

        if (recv_len == -1)
        {
            error("Failed to receive response from server!");
        }

        buffer[recv_len] = '\0';
        std::cout << "[Info] Received response from server: " << buffer << std::endl;
    }
};

int main()
{
    try
    {
        UDPClient client("192.168.182.128", 9999);
        client.init();

        std::string input_msg;
        std::cout << "Enter message to send (type 'quit' to exit): ";
        while(std::getline(std::cin, input_msg))
        {
            if (input_msg == "quit")
            {
                std::cout << "[Info] Client exiting..." << std::endl;
                break;
            }
            client.send_and_receive(input_msg);
            std::cout << "Enter message to send (type 'quit' to exit): ";
        }
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "[Fatal] Client error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}