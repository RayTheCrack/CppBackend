// BBL DRIZZY - 修复版 Select + 多线程 TCP 服务器
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <string>
#include <set>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <thread>
#include <cstring>  // 用于 strerror 获取错误详情

class TCPServer
{
private:
    int max_fd;
    int server_fd;
    fd_set read_fds;
    const uint16_t server_port;
    std::set<int, std::greater<int>> client_fds;
    std::mutex fd_mtx;
    std::atomic<bool> is_running;  // 原子变量控制服务器运行状态

    void error(const std::string& msg, bool CloseServer = true)
    {
        std::unique_lock<std::mutex> lock(fd_mtx);
        std::cerr << "[Error] " << msg << " (" << strerror(errno) << ")" << std::endl;
        // 关闭所有客户端连接
        for(int fd : client_fds) if(fd != -1) close(fd);
        client_fds.clear();
        if(CloseServer and server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        is_running = false;
        exit(EXIT_FAILURE);
    }

public:
    explicit TCPServer(const uint16_t _port) : server_port(_port), server_fd(-1), max_fd(-1), is_running(true)
    {
        FD_ZERO(&read_fds);
    }

    ~TCPServer() noexcept
    {
        is_running = false;  // 停止服务器循环
        std::unique_lock<std::mutex> lock(fd_mtx);
        // 清理所有客户端连接
        for(int fd : client_fds) if(fd != -1) close(fd);
        client_fds.clear();
        // 关闭服务器套接字
        if(server_fd != -1) close(server_fd);
        std::cout << "[Info] Server resources cleaned up" << std::endl;
    }

    void init()
    {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(server_fd == -1)
        {
            error("Failed to create a socket!");
        }
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        int opt = 1;
        if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
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
            error("Failed to listen on socket!");
        }
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;
        std::cout << "Server is currently listening on port: " << server_port << std::endl; 
    }
    // 处理新客户端连接（建议主线程执行，避免多线程accept问题）
    void new_client()
    {
        sockaddr_in client_addr{};
        socklen_t siz = sizeof(client_addr);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &siz);
        if(client_fd == -1)
        {
            // 失败时仅打印警告，不退出服务器
            std::cerr << "[Warning] Failed to accept client connection! " << strerror(errno) << " Continue..." << std::endl;
            return;
        }
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip));
        uint16_t client_port = ntohs(client_addr.sin_port);
        std::cout << "[Info] New client connected: IP = " << client_ip << ", Port = " << client_port << ", FD = " << client_fd << std::endl;
        // 加锁修改共享的fd集合，保证线程安全
        std::unique_lock<std::mutex> lock(fd_mtx);
        FD_SET(client_fd, &read_fds);
        client_fds.insert(client_fd);
        max_fd = std::max(server_fd, *client_fds.begin());
    }
    // 客户端通信逻辑（子线程执行）
    void client_communicate(int fd)
    {
        char buffer[1024]{};
        ssize_t msg_len = recv(fd, buffer, sizeof(buffer) - 1, 0);
        if(msg_len > 0)
        {
            buffer[msg_len] = '\0';
            std::cout << "[Client " << fd << "] message : " << buffer << std::endl;
            // 发送响应
            std::string response = "Server received message : " + std::string(buffer);
            ssize_t resp_len = send(fd, response.c_str(), response.size(), 0);
            if(resp_len == -1)
            {
                std::cerr << "[Warning] Failed to send response to client " << fd << "! " << strerror(errno) << std::endl;
            }
        }
        else
        {
            // 客户端断开或接收失败
            if(msg_len == 0)
                std::cout << "[Info] Client " << fd << " disconnected!" << std::endl;
            else
                std::cerr << "[Warning] Failed to receive data from client " << fd << "! " << strerror(errno) << std::endl;
            // 加锁清理客户端资源
            std::unique_lock<std::mutex> lock(fd_mtx);
            FD_CLR(fd, &read_fds);  // 从读集合移除
            close(fd);              // 关闭fd
            client_fds.erase(fd);   // 从客户端集合移除
            if(client_fds.empty()) max_fd = server_fd;
            else max_fd = std::max(server_fd, *client_fds.begin());
        }
    }
    void select_loop()
    {   
        std::cout << "[Info] Server is waiting for client connections..." << std::endl;
        fd_set temp_fds;
        
        while(is_running)
        {
            // 加锁拷贝read_fds，避免多线程修改导致的集合错乱
            std::unique_lock<std::mutex> lock(fd_mtx);
            temp_fds = read_fds;
            int cur_max_fd = max_fd;  // 拷贝当前max_fd，避免后续修改影响
            lock.unlock();  // 释放锁，避免select阻塞时占用锁
            // 监听读事件（阻塞）
            int cnt = select(cur_max_fd + 1, &temp_fds, nullptr, nullptr, nullptr);
            if(cnt == -1)
            {
                if(!is_running) break;  // 服务器停止时正常退出
                std::cerr << "[Warning] Failed to call select! " << strerror(errno) << " Continue..." << std::endl;
                continue;
            }
            for(int fd=0; fd<=cur_max_fd; fd++)
            {
                if(FD_ISSET(fd, &temp_fds))
                {
                    if(fd == server_fd)
                    {
                        // 新连接：主线程处理（避免多线程accept的兼容性问题）
                        new_client();
                    }
                    else
                    {
                        // 客户端通信：创建子线程处理，避免阻塞select循环
                        std::thread t(&TCPServer::client_communicate, this, fd);
                        t.detach();  // 分离线程，自动回收资源
                    }
                }
            }
        }
    }
    // 停止服务器（用于优雅退出）
    void stop()
    {
        is_running = false;
        // 唤醒select，避免阻塞
        if(server_fd != -1) shutdown(server_fd, SHUT_RD);
    }
};

// 信号处理：支持Ctrl+C优雅退出
#include <signal.h>
TCPServer* global_server = nullptr;

void signal_handler(int sig)
{
    if(global_server and sig == SIGINT)
    {
        std::cout << "\n[Info] Received SIGINT, shutting down server..." << std::endl;
        global_server->stop();
    }
}

int main()
{   
    // 设置SIGINT信号处理（Ctrl+C）
    signal(SIGINT, signal_handler);
    TCPServer server(9999);
    global_server = &server;
    server.init();
    server.select_loop();
    std::cout << "Server stopped successfully!" << std::endl;
    return 0;
}