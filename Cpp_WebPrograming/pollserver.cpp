// BBL DRIZZY
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <string>
#include <set>
#include <algorithm>
#include <signal.h>
#include <atomic>
#include <poll.h>
#include <vector>

class TCPServer
{
private:
    int max_fd;
    int server_fd;
    std::atomic<bool> is_running;
    const uint16_t server_port;
    std::vector<pollfd> fds;
    std::set<int,std::greater<int>> client_fds;

    void error(const std::string& msg, bool CloseServer = true)
    {
        std::cerr << "[Error] " << msg << std::endl;
        for(int fd : client_fds) if(fd != -1) close(fd);
        client_fds.clear();
        fds.clear();
        if(CloseServer and server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        is_running = false;
        exit(EXIT_FAILURE);
    }

public:
    explicit TCPServer(const uint16_t _port) 
        : server_port(_port), server_fd(-1), max_fd(-1), is_running(true) {}

    ~TCPServer() noexcept
    {
        is_running = false;
        for(int fd : client_fds) if(fd != -1) close(fd);
        fds.clear();
        client_fds.clear();
        if(server_fd != -1) close(server_fd);
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
        // 设置端口复用，避免服务器重启时端口占用
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

        fds.resize(server_fd + 1);
        fds[server_fd] = {server_fd, POLLIN, 0};
        max_fd = server_fd;

        std::cout << "Server is currently listening on port: " << server_port << std::endl; 
    }

    void new_client()
    {
        sockaddr_in client_addr{};
        socklen_t siz = sizeof(client_addr);

        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &siz);
        if(client_fd == -1)
        {
            std::cerr << "[Warning] Failed to accept client connection! Continue..." << std::endl;
            return;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip));
        uint16_t client_port = ntohs(client_addr.sin_port);
        std::cout << "[Info] New client connected: IP = " << client_ip << ", Port = " << client_port << std::endl;

        if(client_fd >= fds.size()) fds.resize(client_fd + 1);
        fds[client_fd] = {client_fd, POLLIN, 0};
        client_fds.insert(client_fd);
        max_fd = std::max(server_fd, *client_fds.begin());
    }

    // 单线程执行，移除线程相关逻辑
    void client_communicate(int fd)
    {
        if(!is_running)
        {
            fds[fd] = {-1, 0 ,0};
            close(fd);
            client_fds.erase(fd);
            std::cout << "[Info] Client " << fd << " closed due to server shutdown" << std::endl;
            return;
        }

        char buffer[1024]{};
        ssize_t msg_len = recv(fd, buffer, sizeof(buffer) - 1, 0);
        if(msg_len > 0)
        {
            buffer[msg_len] = '\0';
            std::cout << "[Client " << fd << "] message : " << buffer << std::endl;

            std::string response = "Server received message : " + std::string(buffer);
            ssize_t resp_len = send(fd, response.c_str(), response.size(), 0);
            if(resp_len == -1 and is_running)
            {
                std::cerr << "[Warning] Failed to send response to client " << fd << "!" << std::endl;
            }
        }
        else
        {
            if(msg_len == 0 or !is_running)
                std::cout << "[Info] Client " << fd << " disconnected!" << std::endl;
            else
                std::cerr << "[Warning] Failed to receive data from client " << fd << "!" << std::endl;
            

            fds[fd] = {-1, 0 ,0};
            // 关闭该客户端fd
            close(fd);
            // 从客户端fd容器中移除;
            client_fds.erase(fd);
            if(client_fds.empty()) max_fd = server_fd;
            else max_fd = std::max(server_fd, *client_fds.begin());
        }
    }

    void poll_loop()
    {   
        std::cout << "[Info] Server is waiting for client connections..." << std::endl;
        while(is_running)
        {   

            int cur_max_fd = max_fd;
            
            int cnt = poll(fds.data(), cur_max_fd + 1, -1);
            if(cnt == -1)
            {
                // 先检测是否是服务器正常退出导致的select失败
                if(!is_running) 
                {
                    std::cout << "[Info] Poll exited normally (server shutdown)" << std::endl;
                    break; // 直接退出循环，不打印错误
                }
                // 仅在服务器未退出时打印警告，不调用error()（避免程序退出）
                std::cerr << "[Warning] Failed to call Poll! Continue..." << std::endl;
                continue;
            }
            
            for(int fd=0;fd<=cur_max_fd and cnt > 0;fd++)
            {
                // 跳过无事件/无效的fd
                if(fds[fd].fd == -1 or fds[fd].revents == 0) continue;
                
                if(fds[fd].revents & POLLIN)
                {
                    // 此时说明有新的客户端连接
                    if(fd == server_fd)
                    {
                        new_client();
                    }
                    else // 否则则是客户端进行通信
                    {
                        client_communicate(fd);
                    }
                    cnt--;
                }
            }
        }
    }

    void stop()
    {
        is_running = false;
        if(server_fd != -1)
            shutdown(server_fd, SHUT_RD);
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
    signal(SIGINT, signals);
    TCPServer server(9999);
    ptr = &server;
    server.init();
    server.poll_loop();
    std::cout << "Closed server sucessfully!" << std::endl;
    return 0;
}