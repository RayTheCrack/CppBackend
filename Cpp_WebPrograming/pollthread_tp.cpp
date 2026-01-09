// BBL DRIZZY
#include "threadpool.h"
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <thread>
#include <signal.h>
#include <mutex>
#include <atomic>
#include <poll.h>
#include <vector>
#include <stdexcept>
#include <csignal>

class TCPServer
{
private:
    int server_fd;
    std::atomic<bool> is_running;
    const uint16_t server_port;
    std::vector<pollfd> fds;          // 仅存储有效fd的pollfd，下标无需和fd对应
    std::unordered_set<int> client_fds; 
    std::mutex mtx;
    ThreadPool pool;

    // 查找监听fd在fds中的索引（仅主线程调用，无需加锁）
    int find_listen_fd_idx() const 
    {
        for(size_t i=0;i<fds.size();i++)
            if(fds[i].fd == server_fd) return i;
        return -1;
    }
    // 查找客户端fd在fds中的索引（加锁后调用）
    int find_client_fd_idx(int fd) const
    {
        for(size_t i=0;i<fds.size();i++) 
            if(fds[i].fd == fd)  return i;
        return -1;
    }

    void error(const std::string& msg, bool CloseServer = true)
    {
        std::unique_lock<std::mutex> lock(mtx);
        std::cerr << "[Error] " << msg << std::endl;
        // 清理客户端fd
        for(int fd : client_fds) if(fd != -1) close(fd);
        client_fds.clear();
        fds.clear();
        // 关闭服务器fd
        if(CloseServer && server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        is_running = false;
        throw std::runtime_error(msg);
    }

public:
    explicit TCPServer(const uint16_t _port) 
        : server_port(_port), server_fd(-1), is_running(true), pool(10) {}

    ~TCPServer() noexcept
    {
        stop();
        std::unique_lock<std::mutex> lock(mtx);
        // 清理所有客户端fd
        for(int fd : client_fds) if(fd != -1) close(fd);
        client_fds.clear();
        fds.clear();
        // 关闭服务器fd
        if(server_fd != -1) 
        {
            close(server_fd);
            server_fd = -1;
        }
        std::cout << "[Info] Server resources cleaned up" << std::endl;
    }

    void init()
    {
        // 创建监听socket
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(server_fd == -1)
        {
            error("Failed to create a socket!");
        }

        // 设置端口复用
        int opt = 1;
        if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        {
            error("Failed to set socket options!");
        }
        
        // 绑定地址
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        if(bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1)
        {
            error("Failed to bind socket!");
        }

        // 开始监听
        if(listen(server_fd, 128) == -1)
        {
            error("Failed to listen on socket!");
        }

        // 初始化pollfd数组：仅加入监听fd
        fds.push_back({server_fd, POLLIN, 0});
        std::cout << "Server is currently listening on port: " << server_port << std::endl; 
    }

    void new_client()
    {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if(client_fd == -1)
        {
            std::cerr << "[Warning] Failed to accept client connection! Continue..." << std::endl;
            return;
        }

        std::unique_lock<std::mutex> lock(mtx);
        // 将新客户端fd加入poll监听和客户端集合
        fds.push_back({client_fd, POLLIN, 0});
        client_fds.insert(client_fd);
        std::cout << "[Info] New client connected: " << client_fd << " (total clients: " << client_fds.size() << ")" << std::endl;
    }

    void client_communicate(int fd)
    {
        char buffer[1024]{};
        ssize_t len = recv(fd, buffer, sizeof(buffer) - 1, 0);
        
        // 客户端断开或读取失败
        if(len <= 0)
        {
            std::cout << "[Info] Client " << fd << " disconnected" << std::endl;
            std::unique_lock<std::mutex> lock(mtx);
            // 关闭fd并清理资源
            close(fd);
            int idx = find_client_fd_idx(fd);
            if(idx != -1) fds.erase(fds.begin() + idx);
            client_fds.erase(fd);
            return;
        }

        // 读取到数据，提交到线程池处理
        std::string msg(buffer, len);
        pool.submit([this, fd, msg]() 
        {
            std::string resp = "Server received: " + msg;
            // 检查服务器状态，避免关闭后发送
            if(is_running) 
            {
                ssize_t send_len = send(fd, resp.c_str(), resp.size(), 0);
                if(send_len == -1) 
                {
                    std::cerr << "[Warning] Failed to send to client " << fd << std::endl;
                }
            }

        });
    }

    void poll_loop()
    {   
        std::cout << "[Info] Server is waiting for client connections..." << std::endl;
        while(is_running)
        {   
            int cnt = poll(fds.data(),fds.size(),1000); // 1秒超时，避免永久阻塞
            // poll调用失败
            if(cnt == -1)
            {
                if(!is_running) 
                {
                    std::cout << "[Info] Poll exited normally (server shutdown)" << std::endl;
                    break;
                }
                std::cerr << "[Warning] Failed to call Poll! Continue..." << std::endl;
                continue;
            }
            if(cnt == 0) continue;
            // 遍历所有pollfd，处理事件（复制fds避免遍历中修改）
            std::vector<pollfd> fds_copy;
            {
                std::unique_lock<std::mutex> lock(mtx);
                fds_copy = fds;
            }
            for(const auto& pfd : fds_copy)
            {
                if(pfd.revents == 0) continue;
                // 处理读事件
                if(pfd.revents & POLLIN)
                {
                    if(pfd.fd == server_fd) 
                    {
                        new_client(); // 新连接
                    } 
                    else 
                    {
                        client_communicate(pfd.fd); // 客户端数据
                    }
                }

                // 处理错误事件
                if(pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) 
                {
                    std::cout << "[Info] Client " << pfd.fd << " error, disconnecting" << std::endl;
                    std::unique_lock<std::mutex> lock(mtx);
                    close(pfd.fd);
                    int idx = find_client_fd_idx(pfd.fd);
                    if(idx != -1) fds.erase(fds.begin() + idx);
                    client_fds.erase(pfd.fd);
                }
            }
        }
    }

    void stop()
    {
        if(!is_running) return; // 避免重复停止
        std::cout << "[Info] Starting server shutdown..." << std::endl;
        is_running = false;
        // 关闭监听fd的读端，触发poll退出
        if(server_fd != -1) {
            shutdown(server_fd, SHUT_RDWR); // 关闭读写，避免新连接/数据
        }
        // 等待线程池任务完成
        std::unique_lock<std::mutex> endlock(pool.end_mtx);
        pool.end_cv.wait(endlock,[this](){return pool.task_count == 0;});
    }
};

TCPServer* ptr = nullptr;

void signals(int sig)
{
    if(ptr and sig == SIGINT)
    {
        std::cout << "\n[Info] Received SIGINT, shutting down server..." << std::endl;
        ptr->stop();
    }
}

int main()
{   
    // 忽略SIGPIPE信号，避免send失败导致进程退出，全局声明
    signal(SIGPIPE, SIG_IGN);

    signal(SIGINT, signals);
    try 
    {
        TCPServer server(9999);
        ptr = &server;
        server.init();
        server.poll_loop();
        std::cout << "Closed server successfully!" << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << "[Fatal] Server exited with error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}