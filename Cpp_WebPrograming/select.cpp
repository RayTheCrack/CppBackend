#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <vector>
#include <set>

class TCPServer 
{
private:
    int server_fd;
    const uint16_t port;
    fd_set read_fds;  // select监听的读事件集合
    int max_fd;       // 记录最大文件描述符，用于select第一个参数
    std::set<int,std::greater<int>> client_fds;  // 存储所有客户端fd，便于管理
    void error(const std::string& msg, bool CloseServer = true)
    {
        std::cerr << "[Error] " << msg << std::endl;
        // 关闭所有客户端fd
        for(int fd : client_fds) if (fd != -1) close(fd);
        // 关闭服务器fd
        if(CloseServer and server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        exit(EXIT_FAILURE);
    }

public:
    // 构造函数：初始化端口，初始化套接字为无效值
    explicit TCPServer(const uint16_t _port) : port(_port), server_fd(-1), max_fd(-1) 
    {
        FD_ZERO(&read_fds); // 初始化读集合为空
    }

    ~TCPServer() noexcept
    {
        // 关闭所有客户端fd
        for(int fd : client_fds) if (fd != -1) close(fd);
        // 关闭服务器fd
        if(server_fd != -1) close(server_fd);
        std::cout << "[Info] TCPServer destroyed!" << std::endl;
    }
    // 初始化服务器，创建套接字->绑定->监听
    void init()
    {
        // 创建服务器套接字
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(server_fd == -1)
        {
            error("Failed to create a socket!");
        }

        // 设置端口复用，避免服务器重启时端口占用
        int opt = 1;
        if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) 
        {
            error("Failed to set socket options!");
        }
        // 绑定地址和端口
        sockaddr_in server_addr{};  
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        int ret = bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
        if(ret == -1)
        {
            error("Failed to bind socket!");
        }

        // 开始监听
        int lis = listen(server_fd, 128);
        if(lis == -1)
        {
            error("Failed to set listen socket!");
        }

        // 初始化select相关参数
        FD_SET(server_fd, &read_fds);  // 将服务器监听fd加入读集合
        max_fd = server_fd;            // 更新最大fd为服务器fd

        std::cout << "Server initialized successful! Listening on port: " << port << std::endl;
    }

    // 使用select循环监听并处理多客户端事件（连接+数据通信）
    void run_select_loop()
    {
        std::cout << "[Info] Server is waiting for client connections..." << std::endl;
        fd_set temp_read_fds;  // 临时读集合（select会修改集合，需每次重置）

        while(true)
        {
            // 每次循环重置临时读集合（原始集合保持不变，用于重置）
            temp_read_fds = read_fds;

            // 调用select，阻塞监听读事件
            // 参数：max_fd+1 | 读集合 | 写集合（nullptr） | 异常集合（nullptr） | 超时（nullptr永久阻塞）
            int active_fd_count = select(max_fd + 1, &temp_read_fds, nullptr, nullptr, nullptr);
            if(active_fd_count == -1)
            {
                error("Failed to call select!", false);
                continue;
            }
            // 遍历所有可能的fd，处理触发事件的fd
            for(int fd=0;fd<=max_fd;fd++)
            {
                // 判断当前fd是否在触发事件的临时集合中
                if(FD_ISSET(fd, &temp_read_fds))
                {
                    // 情况1：服务器监听fd触发事件 → 新客户端连接
                    if(fd == server_fd)
                    {
                        sockaddr_in client_addr{};
                        socklen_t siz = sizeof(client_addr);
                        int new_client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &siz);
                        if(new_client_fd == -1)
                        {
                            std::cerr << "[Warning] Failed to accept client connection! Continue..." << std::endl;
                            continue;
                        }
                        // 打印客户端信息
                        char client_ip[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip));
                        uint16_t client_port = ntohs(client_addr.sin_port);
                        std::cout << "[Info] New client connected: IP = " << client_ip << ", Port = " << client_port << std::endl;
                        // 将新客户端fd加入原始读集合
                        FD_SET(new_client_fd, &read_fds);
                        // 将新客户端fd存入管理容器
                        client_fds.insert(new_client_fd);
                        // 更新最大fd
                        max_fd = std::max(server_fd,*client_fds.begin());
                    }
                    // 情况2：客户端fd触发事件 → 客户端发送数据/断开连接
                    else
                    {
                        char buffer[1024]{};
                        ssize_t msg_len = recv(fd, buffer, sizeof(buffer) - 1, 0);
                        // 子情况1：读取到有效数据
                        if(msg_len > 0)
                        {
                            buffer[msg_len] = '\0';
                            std::cout << "[Client " << fd << "] message : " << buffer << std::endl;
                            // 发送响应给客户端
                            std::string response = "Message received : " + std::string(buffer);
                            ssize_t respon_len = send(fd, response.c_str(), response.size(), 0);
                            if(respon_len == -1)
                            {
                                std::cerr << "[Warning] Failed to send response to client " << fd << "!" << std::endl;
                            }
                        }
                        else
                        {
                            if(msg_len == 0)
                                std::cout << "[Info] Client " << fd << " disconnected!" << std::endl;
                            else 
                                std::cerr << "[Warning] Failed to receive data from client " << fd << "!" << std::endl;
                            // 从读集合中移除该fd
                            FD_CLR(fd, &read_fds);
                            // 关闭该客户端fd
                            close(fd);
                            // 从客户端fd容器中移除
                            client_fds.erase(fd);
                            if(client_fds.empty()) max_fd = server_fd;
                            else max_fd = std::max(server_fd,*client_fds.begin());
                        }
                    }
                }
            }
        }
    }
};

int main()
{
    TCPServer server(9999);
    server.init();
    server.run_select_loop(); // 启动select循环，处理多客户端并发
    std::cout << "The server exited successfully!" << std::endl;
    return EXIT_SUCCESS;
}