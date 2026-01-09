// BBL DRIZZY
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <string>
#include <set>
#include <algorithm>
#include <thread>
#include <signal.h>
#include <mutex>
#include <atomic>
#include <poll.h>
#include <vector>
#include <sys/epoll.h>

class TCPServer
{
private:
    int server_fd;
    std::atomic<bool> is_running;
    const uint16_t server_port;
    int epfd; // epoll实例fd（替代poll的fds数组）
    std::mutex mtx;
    

    void error(const std::string& msg, bool CloseServer = true)
    {
        std::unique_lock<std::mutex> lock(mtx);
        std::cerr << "[Error] " << msg << std::endl;
        if(CloseServer and server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        if(epfd != -1) close(epfd);
        is_running = false;
        exit(EXIT_FAILURE);
    }

public:

    explicit TCPServer(const uint16_t _port) 
        : server_port(_port), server_fd(-1), epfd(-1), is_running(true) {}

    ~TCPServer() noexcept
    {
        is_running = false;
        std::unique_lock<std::mutex> lock(mtx);
        if(server_fd != -1) close(server_fd);
        if(epfd != -1) close(epfd);
        std::cout << "[Info] TCPServer destoryed!" << std::endl;
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
        // 创建epoll实例
        epfd = epoll_create1(EPOLL_CLOEXEC);
        if(epfd == -1)
        {
            error("Failed to create epoll instance!");
        }
        // 将server_fd添入epoll事件表中（监听读事件）
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = server_fd;
        if(epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
        {
            error("Failed to add server_fd to epoll!");
        }
        std::cout << "[Info] Server is currently listening on port: " << server_port << " (epoll mode)" << std::endl; 
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

        std::unique_lock<std::mutex> lock(mtx);
        epoll_event client_event;
        client_event.events = EPOLLIN;
        client_event.data.fd = client_fd;
        if(epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_event) == -1)
        {
            std::cerr << "[Warning] Failed to add client_fd to epoll!" << std::endl;
            close(client_fd);
            return;
        }
    }

    void client_communicate(int fd)
    {
        if(!is_running)
        {
            std::unique_lock<std::mutex> lock(mtx);
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
            std::cout << "[Info] Client " << fd << " closed due to server shutdown" << std::endl;
            return;
        }

        char buffer[1024]{};
        ssize_t msg_len = recv(fd, buffer, sizeof(buffer) - 1,0);
        if(msg_len > 0)
        {
            buffer[msg_len] = '\0';
            std::cout << "[Client " << fd << "] message : " << buffer << std::endl;

            std::string response = "Server received message : " + std::string(buffer);
            ssize_t resp_len = send(fd, response.c_str(), response.size(), 0);
            if(resp_len == -1 and is_running)
            {
                std::cerr << "[Warning] Failed to send response to client " << fd << "!" << std::endl;
                // 新增：发送失败时清理fd
                std::unique_lock<std::mutex> lock(mtx);
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                close(fd);
                return;
            }
        }
        else
        {
            if(msg_len == 0 or !is_running)
                std::cout << "[Info] Client " << fd << " disconnected!" << std::endl;
            else
                std::cerr << "[Warning] Failed to receive data from client " << fd << "!" << std::endl;
            
            std::unique_lock<std::mutex> lock(mtx);
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);  // 从epoll移除fd
            close(fd);
        }
    }

    void epoll_loop()
    {   
        std::cout << "[Info] Server is waiting for client connections..." << std::endl;
        int MAX_EVENTS = 100010;
        std::vector<epoll_event> events(MAX_EVENTS);
        
        while(is_running)
        {   
            int nfds = epoll_wait(epfd, events.data(), MAX_EVENTS, -1);
            if(nfds == -1)
            {
                if(!is_running) 
                {
                    std::cout << "[Info] Epoll exited normally (server shutdown)" << std::endl;
                    break;
                }
                std::cerr << "[Warning] Failed to call epoll_wait! Continue..." << std::endl;
                continue;
            }

            for(int i=0;i<nfds;i++)
            {
                int fd = events[i].data.fd;
                if(events[i].events & EPOLLIN)
                {
                    if(fd == server_fd)
                    {
                        new_client();
                    }
                    else
                    {
                        std::thread t(&TCPServer::client_communicate, this, fd);
                        t.detach();
                    }
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
    server.epoll_loop();
    std::cout << "Closed server sucessfully!" << std::endl;
    return 0;
}