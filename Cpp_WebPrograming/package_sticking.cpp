#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdint.h> // 固定长度整数（uint32_t）

// 发送带长度前缀的数据包（解决粘包：先送长度，再送数据）
bool send_packet(int client_fd, const std::string& data) {
    // 步骤1：计算数据长度，转换为网络字节序（跨平台）
    uint32_t data_len = htonl(data.size()); // htonl：主机序转网络序（大端）
    
    // 步骤2：发送长度前缀（4字节）
    ssize_t len_sent = send(client_fd, &data_len, sizeof(data_len), 0);
    if (len_sent != sizeof(data_len)) {
        std::cerr << "发送长度前缀失败" << std::endl;
        return false;
    }
    
    // 步骤3：发送实际数据
    len_sent = send(client_fd, data.c_str(), data.size(), 0);
    if (len_sent != data.size()) {
        std::cerr << "发送数据失败" << std::endl;
        return false;
    }
    return true;
}

// 接收带长度前缀的数据包（解决粘包：先读长度，再读数据）
bool recv_packet(int client_fd, std::string& out_data) {
    out_data.clear();
    
    // 步骤1：读取长度前缀（4字节）
    uint32_t data_len = 0;
    ssize_t len_read = recv(client_fd, &data_len, sizeof(data_len), 0);
    if (len_read <= 0) { // 连接断开或出错
        return false;
    }
    data_len = ntohl(data_len); // 网络序转主机序
    
    // 步骤2：按长度读取实际数据（处理拆包：可能需要多次recv）
    char buffer[4096] = {0};
    uint32_t total_read = 0;
    while (total_read < data_len) {
        len_read = recv(client_fd, buffer, std::min((uint32_t)sizeof(buffer), data_len - total_read), 0);
        if (len_read <= 0) {
            return false;
        }
        out_data.append(buffer, len_read);
        total_read += len_read;
    }
    return true;
}

// 极简 TCP 服务器（演示粘包解决方案）
int main() 
{
    // 1. 创建 TCP 套接字
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    // 2. 绑定端口
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9999);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(server_fd);
        return -1;
    }
    
    // 3. 监听
    if (listen(server_fd, 128) == -1) {
        perror("listen failed");
        close(server_fd);
        return -1;
    }
    std::cout << "TCP 服务器启动，监听 9999 端口..." << std::endl;
    
    // 4. 接受客户端连接
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_fd == -1) {
        perror("accept failed");
        close(server_fd);
        return -1;
    }
    std::cout << "客户端 " << inet_ntoa(client_addr.sin_addr) << " 连接成功" << std::endl;
    
    // 5. 通信：接收客户端数据包（无粘包）
    std::string recv_data;
    while(true) {
        if (recv_packet(client_fd, recv_data)) {
            std::cout << "收到客户端数据：" << recv_data << std::endl;
            // 回复客户端（同样带长度前缀）
            send_packet(client_fd, "服务器已收到：" + recv_data);
        } else {
            std::cout << "客户端断开连接" << std::endl;
            break;
        }
    }
    
    // 6. 关闭连接
    close(client_fd);
    close(server_fd);
    return 0;
}