#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdint.h>
#include <algorithm>

// 复用服务器的send_packet和recv_packet函数（逻辑完全一致）
bool send_packet(int sock_fd, const std::string& data) {
    uint32_t data_len = htonl(static_cast<uint32_t>(data.size()));
    ssize_t len_sent = send(sock_fd, &data_len, sizeof(data_len), 0);
    if (len_sent != sizeof(data_len)) {
        std::cerr << "发送长度前缀失败：" << strerror(errno) << std::endl;
        return false;
    }
    const char* data_ptr = data.c_str();
    uint32_t remaining = data.size();
    while (remaining > 0) {
        len_sent = send(sock_fd, data_ptr, remaining, 0);
        if (len_sent <= 0) {
            std::cerr << "发送数据失败：" << strerror(errno) << std::endl;
            return false;
        }
        remaining -= len_sent;
        data_ptr += len_sent;
    }
    return true;
}

bool recv_packet(int sock_fd, std::string& out_data) {
    out_data.clear();
    uint32_t data_len = 0;
    char* len_buf = reinterpret_cast<char*>(&data_len);
    uint32_t len_remaining = sizeof(data_len);
    while (len_remaining > 0) {
        ssize_t len_read = recv(sock_fd, len_buf, len_remaining, 0);
        if (len_read == 0) return false;
        if (len_read < 0) {
            std::cerr << "读取长度前缀失败：" << strerror(errno) << std::endl;
            return false;
        }
        len_remaining -= len_read;
        len_buf += len_read;
    }
    data_len = ntohl(data_len);
    if (data_len > 1024 * 1024) return false;
    
    out_data.reserve(data_len);
    char buffer[1024] = {0};
    uint32_t total_read = 0;
    while (total_read < data_len) {
        uint32_t read_size = std::min(static_cast<uint32_t>(sizeof(buffer)), data_len - total_read);
        ssize_t len_read = recv(sock_fd, buffer, read_size, 0);
        if (len_read == 0) return false;
        if (len_read < 0) {
            std::cerr << "读取数据失败：" << strerror(errno) << std::endl;
            return false;
        }
        out_data.append(buffer, len_read);
        total_read += len_read;
    }
    return true;
}

int main() {
    // 1. 创建套接字
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket failed");
        return -1;
    }
    
    // 2. 连接服务器
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9999);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(sock_fd);
        return -1;
    }
    if (connect(sock_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        perror("connect failed");
        close(sock_fd);
        return -1;
    }
    std::cout << "连接服务器成功，请输入消息（输入exit退出）：" << std::endl;
    
    // 3. 循环发送数据
    std::string input;
    std::string recv_data;
    while (true) {
        std::cout << "客户端> ";
        std::getline(std::cin, input);
        if (input == "exit") break;
        
        // 发送带长度前缀的数据包
        if (!send_packet(sock_fd, input)) {
            std::cerr << "发送失败，断开连接" << std::endl;
            break;
        }
        
        // 接收服务器回复
        if (recv_packet(sock_fd, recv_data)) {
            std::cout << "服务器> " << recv_data << std::endl;
        } else {
            std::cerr << "服务器断开连接" << std::endl;
            break;
        }
    }
    
    // 4. 关闭连接
    close(sock_fd);
    return 0;
}