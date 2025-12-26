#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cerrno>
#include <stdexcept>  // C++ 异常类

// 封装TCP客户端类，符合C++面向对象设计
class TCPClient {
private:
    int client_fd_;               // 客户端套接字
    std::string server_ip_;       // 服务器IP
    uint16_t server_port_;        // 服务器端口

    // 统一错误处理函数（C++风格）
    void handleError(const std::string& msg, bool closeClient = true) {
        std::cerr << "[Error] " << msg 
                  << " (errno: " << errno << ", detail: " << strerror(errno) << ")" << std::endl;
        if (closeClient && client_fd_ != -1) {
            close(client_fd_);
            client_fd_ = -1;
        }
        throw std::runtime_error(msg);  // 抛出异常，上层可捕获
    }

public:
    // 构造函数：初始化服务器地址和端口，套接字置为无效值
    TCPClient(const std::string& server_ip, uint16_t server_port)
        : server_ip_(server_ip), server_port_(server_port), client_fd_(-1) {}

    // 析构函数：确保套接字关闭（RAII思想，防止资源泄漏）
    ~TCPClient() {
        if (client_fd_ != -1) {
            close(client_fd_);
            std::cout << "[Info] Client socket closed successfully" << std::endl;
        }
    }

    // 初始化客户端：创建套接字 + 连接服务器
    void connectToServer() {
        // 1. 创建通信套接字（SOCK_STREAM = TCP）
        client_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fd_ == -1) {
            handleError("Failed to create client socket");
        }

        // 2. 配置服务器地址结构体（C++11空初始化，避免野值）
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port_);

        // 转换IP地址（主机字节序 → 网络字节序）
        if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr.s_addr) <= 0) {
            handleError("Invalid server IP address: " + server_ip_);
        }

        // 3. 连接服务器（C++规范的类型转换）
        std::cout << "[Info] Connecting to server " << server_ip_ << ":" << server_port_ << "..." << std::endl;
        int ret = connect(client_fd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
        if (ret == -1) {
            // 修复原代码错误：这里是connect失败，不是bind失败
            handleError("Failed to connect to server");
        }

        std::cout << "[Info] Connected to server successfully!" << std::endl;
    }

    // 与服务器通信：循环发送/接收数据
    void communicate() {
        if (client_fd_ == -1) {
            handleError("Client not connected to server", false);
        }

        std::string send_msg;
        char recv_buffer[1024]{};  // C++空初始化，避免脏数据

        while (true) {
            // 4. 输入要发送的消息（C++ getline 处理带空格的输入）
            std::cout << "Enter message to send (empty to exit): ";
            std::getline(std::cin, send_msg);

            // 空输入退出循环（友好退出逻辑）
            if (send_msg.empty()) {
                std::cout << "[Info] User exited, closing connection..." << std::endl;
                break;
            }

            // 发送数据到服务器
            ssize_t send_len = send(client_fd_, send_msg.c_str(), send_msg.size(), 0);
            if (send_len == -1) {
                handleError("Failed to send data to server", false);
                break;
            }
            std::cout << "[Info] Sent " << send_len << " bytes: " << send_msg << std::endl;

            // 接收服务器响应
            ssize_t recv_len = recv(client_fd_, recv_buffer, sizeof(recv_buffer) - 1, 0);
            if (recv_len > 0) {
                recv_buffer[recv_len] = '\0';  // 确保字符串以'\0'结尾，避免乱码
                std::cout << "[Info] Received from server: " << recv_buffer << std::endl;
            } else if (recv_len == 0) {
                std::cout << "[Info] Server closed the connection" << std::endl;
                break;
            } else {
                handleError("Failed to receive data from server", false);
                break;
            }

            // 清空缓冲区，避免下一次接收残留数据
            std::memset(recv_buffer, 0, sizeof(recv_buffer));
        }
    }
};

// 主函数：C++风格入口，异常捕获
int main() {
    try {
        // 配置服务器地址（可根据实际情况修改）
        const std::string server_ip = "192.168.182.128";
        const uint16_t server_port = 9999;

        // 创建客户端对象并执行逻辑
        TCPClient client(server_ip, server_port);
        client.connectToServer();  // 连接服务器
        client.communicate();      // 开始通信

    } catch (const std::runtime_error& e) {
        // 捕获自定义异常，优雅退出
        std::cerr << "[Fatal] Program exited with error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[Info] Client exited normally" << std::endl;
    return EXIT_SUCCESS;
}