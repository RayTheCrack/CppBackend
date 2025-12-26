#include <iostream>
#include <string>
#include <cstring>   // 兼容C风格字符串操作
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cerrno>    // C++ 错误码头文件

// 封装TCP服务端类，更符合C++面向对象风格
class TCPServer 
{
private:
    int server_fd_;          // 监听套接字
    int client_fd_;          // 客户端连接套接字
    const uint16_t port_;    // 监听端口

    // 错误处理封装（C++风格）
    void handleError(const std::string& msg, bool closeServer = true) 
    {
        // 结合errno和自定义信息输出错误
        std::cerr << "Error: " << msg << " (errno: " << errno << ", " << strerror(errno) << ")" << std::endl;
        if (closeServer && server_fd_ != -1) {
            close(server_fd_);
            server_fd_ = -1;
        }
        if (client_fd_ != -1) {
            close(client_fd_);
            client_fd_ = -1;
        }
        exit(EXIT_FAILURE);
    }

public:
    // 构造函数：初始化端口，初始化套接字为无效值
    explicit TCPServer(uint16_t port) : port_(port), server_fd_(-1), client_fd_(-1) {}

    // 析构函数：确保套接字关闭
    ~TCPServer() {
        if (server_fd_ != -1) close(server_fd_);
        if (client_fd_ != -1) close(client_fd_);
    }

    // 初始化服务器（创建套接字、绑定、监听）
    void init() {
        // 1. 创建监听套接字（C++风格的常量命名，类型更清晰）
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ == -1) {
            handleError("Failed to create socket");
        }

        // 2. 绑定IP和端口
        sockaddr_in server_addr{};  // C++11 空初始化
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        server_addr.sin_addr.s_addr = INADDR_ANY;  // 绑定所有可用网卡

        if (bind(server_fd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
            handleError("Failed to bind socket");
        }

        // 3. 监听套接字（backlog设为128）
        if (listen(server_fd_, 128) == -1) {
            handleError("Failed to listen on socket");
        }

        std::cout << "Server started successfully, listening on port: " << port_ << std::endl;
    }

    // 等待并接受客户端连接
    void acceptClient() {
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);

        // 4. 阻塞等待客户端连接（reinterpret_cast符合C++类型转换规范）
        client_fd_ = accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);
        if (client_fd_ == -1) {
            handleError("Failed to accept client connection");
        }

        // 打印客户端信息（C++字符串拼接更优雅）
        char client_ip[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip));
        uint16_t client_port = ntohs(client_addr.sin_port);
        std::cout << "Client connected: IP = " << client_ip << ", Port = " << client_port << std::endl;
    }

    // 与客户端通信
    void communicate() {
        char buffer[1024]{};  // C++空初始化
        while (true) {
            // 5. 接收客户端数据
            ssize_t recv_len = recv(client_fd_, buffer, sizeof(buffer) - 1, 0);  // 留1位存'\0'
            if (recv_len > 0) {
                buffer[recv_len] = '\0';  // 确保字符串以'\0'结尾（C++安全处理）
                std::cout << "Client message: " << buffer << std::endl;

                // 发送响应（C++ string 替代C风格字符数组）
                std::string response = "Message received: " + std::string(buffer);
                ssize_t send_len = send(client_fd_, response.c_str(), response.size(), 0);
                if (send_len == -1) {
                    std::cerr << "Warning: Failed to send response to client" << std::endl;
                    break;
                }
            } else if (recv_len == 0) {
                std::cout << "Client disconnected normally" << std::endl;
                break;
            } else {
                handleError("Failed to receive data from client", false);  // 不关闭server_fd，仅退出通信
                break;
            }
        }
    }
};

// 主函数（C++风格入口）
int main() 
{
    try 
    {
        // 创建服务端对象，监听9999端口
        TCPServer server(9999);
        server.init();          // 初始化服务器
        server.acceptClient();  // 等待客户端连接
        server.communicate();   // 与客户端通信
    } catch (const std::exception& e) {
        // 捕获可能的异常（扩展用）
        std::cerr << "Unexpected exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Server closed successfully" << std::endl;
    return EXIT_SUCCESS;
}