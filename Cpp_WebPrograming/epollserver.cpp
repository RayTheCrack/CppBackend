#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <atomic>
#include <sys/epoll.h>
#include <vector>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

class SingleThreadEPollServer
{
private:
    int server_fd;
    std::atomic<bool> is_running;
    const uint16_t server_port;
    int epfd;

    void error(const std::string& msg, bool CloseServer = true)
    {
        std::cerr << "[Error] " << msg << std::endl;
        if(CloseServer and server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        if(epfd != -1) close(epfd);
        is_running = false;
        throw std::runtime_error(msg);
    }
public:
    explicit SingleThreadEPollServer(const uint16_t _port) 
        : server_port(_port), server_fd(-1), epfd(-1), is_running(true) {}

    ~SingleThreadEPollServer() noexcept
    {
        is_running = false;
        if(server_fd != -1) close(server_fd);
        if(epfd != -1) close(epfd);
        std::cout << "[Info] EPollServer destoryed!" << std::endl;
    }

    void init()
    {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(server_fd == -1) error("Failed to create a socket!");

        // 端口复用
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

        // 移除：server_fd非阻塞设置（默认阻塞）

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        if(bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1)
            error("Failed to bind socket!");

        // 增大监听队列（和poll保持一致）
        if(listen(server_fd, 128) == -1) error("Failed to listen on socket!");

        // 创建epoll实例
        epfd = epoll_create1(0);
        if(epfd == -1) error("Failed to create epoll instance!");

        // 注册server_fd（LT模式，和poll保持一致）
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = server_fd;
        if(epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
            error("Failed to add server_fd to epoll!");

        std::cout << "[Info] Single-thread EPoll Server (Blocking Mode) listening on port: " << server_port << std::endl; 
    }

    void epoll_loop()
    {   
        std::cout << "[Info] Server waiting for connections..." << std::endl;
        const int MAX_EVENTS = 100010;
        std::vector<epoll_event> events(MAX_EVENTS);
        char buffer[1024]{};

        while(is_running)
        {   
            int nfds = epoll_wait(epfd, events.data(), MAX_EVENTS, -1);
            if(nfds == -1)
            {
                if(!is_running) break;
                std::cerr << "[Warning] epoll_wait failed! errno: " << errno << std::endl;
                continue;
            }

            for(int i=0;i<nfds;i++)
            {
                int fd = events[i].data.fd;
                // 新客户端连接
                if(fd == server_fd)
                {
                    // 改为普通阻塞accept（移除accept4和非阻塞循环）
                    sockaddr_in client_addr{};
                    socklen_t siz = sizeof(client_addr);
                    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &siz);
                    if(client_fd == -1)
                    {
                        // std::cerr << "[Warning] accept failed! errno: " << errno << std::endl;
                        continue;
                    }

                    // 注册客户端fd（LT模式，阻塞）
                    epoll_event client_ev;
                    client_ev.events = EPOLLIN;
                    client_ev.data.fd = client_fd;
                    if(epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_ev) == -1) {
                        std::cerr << "[Warning] add client_fd to epoll failed! errno: " << errno << std::endl;
                        close(client_fd);
                        continue;
                    }

                    // // 可选：关闭日志输出提升性能
                    // char client_ip[INET_ADDRSTRLEN];
                    // inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
                    // uint16_t client_port = ntohs(client_addr.sin_port);
                    // std::cout << "[Info] New client connected: " << client_ip << ":" << client_port << std::endl;
                }
                // 客户端数据可读
                else if(events[i].events & EPOLLIN)
                {
                    memset(buffer, 0, sizeof(buffer));
                    // 阻塞recv（无需处理EAGAIN）
                    ssize_t msg_len = recv(fd, buffer, sizeof(buffer)-1, 0);
                    if(msg_len > 0)
                    {
                        buffer[msg_len] = '\0';
                        // 可选：关闭日志输出提升性能
                        // std::cout << "[Client " << fd << "] Received: " << std::string(buffer) << std::endl;

                        // 阻塞send（无需safe_send，无需处理EAGAIN）
                        std::string response = "Server received: " + std::string(buffer);
                        ssize_t resp_len = send(fd, response.c_str(), response.size(), MSG_NOSIGNAL);
                        if(resp_len == -1) {
                            std::cerr << "[Warning] send failed to client " << fd << "! errno: " << errno << std::endl;
                            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                            close(fd);
                        } else {
                            // 可选：关闭日志输出提升性能
                            // std::cout << "[Client " << fd << "] Sent: " << response << std::endl;
                        }
                    }
                    else if(msg_len == 0)
                    {
                        // 客户端正常断开
                        // std::cout << "[Info] Client " << fd << " disconnected!" << std::endl;
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                        close(fd);
                    }
                    else
                    {
                        std::cerr << "[Warning] recv failed from client " << fd << "! errno: " << errno << std::endl;
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                        close(fd);
                    }
                }
            }
        }
    }

    void stop()
    {
        is_running = false;
        if(server_fd != -1) shutdown(server_fd, SHUT_RDWR);
    }
};

SingleThreadEPollServer* ptr = nullptr;

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
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signals);
    
    SingleThreadEPollServer server(9999);
    ptr = &server;
    server.init();
    server.epoll_loop();
    
    std::cout << "Server closed successfully!" << std::endl;
    return 0;
}