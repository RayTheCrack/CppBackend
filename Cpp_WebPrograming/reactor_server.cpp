// reactor_server.cpp
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <iostream>
#include <functional>
#include <unordered_map>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <string>

#include "threadpool.h"

int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// 事件类
class Channel {
    using Callback = std::function<void()>; 

private:
    int __fd; // 当前事件的fd
    Callback __read_cb; // 当前事件若触发读事件，所执行的回调函数

public:
    explicit Channel(int fd) : __fd(fd) {}

    void setReadCallback(Callback cb) { __read_cb = std::move(cb); }

    void handle(uint32_t events)
    {
        // 如果当前事件可读且存在有效回调函数，则调用回调函数
        if((events & EPOLLIN) and __read_cb) __read_cb();
    }

    int fd() const { return __fd; }
};

// Reactor本体->Epoll的封装
class EventLoop {

private:
    int __epfd;
    std::atomic<bool> __running{true};
    epoll_event __events[1024]; // epoll事件表，设定最大监听事件数为1024

public:
    EventLoop()
    {
        __epfd = epoll_create1(EPOLL_CLOEXEC); // 创建epoll实例
        if (__epfd == -1) {
            throw std::runtime_error("epoll_create failed");
        }
    }

    ~EventLoop()
    {
        if(__epfd != -1) close(__epfd);
    }

    // 添加监听事件
    void addChannel(Channel* ch)
    {
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.ptr = ch;
        epoll_ctl(__epfd, EPOLL_CTL_ADD, ch->fd(), &ev);
    }

    // 删除监听事件
    void removeChannel(Channel* ch)
    {
        epoll_ctl(__epfd, EPOLL_CTL_DEL, ch->fd(), nullptr);
    }

    void loop()
    {
        while (__running) {
            int n = epoll_wait(__epfd, __events, 1024, -1);
            for (int i = 0; i < n; ++i) {
                auto* ch = static_cast<Channel*>(__events[i].data.ptr);
                ch->handle(__events[i].events);
            }
        }
    }

    void stop() { __running = false; }
};

// 客户端连接
class Connection {

private:
    EventLoop* __loop;
    int __fd;
    ThreadPool& __pool;
    Channel __channel;

public:
    Connection(EventLoop* loop, int fd, ThreadPool& pool)
        : __loop(loop), __fd(fd), __pool(pool), __channel(fd)
    {
        setNonBlocking(__fd);
        __channel.setReadCallback([this]() { handleRead(); });
        __loop->addChannel(&__channel);
    }

    ~Connection()
    {
        close(__fd);
    }

    void handleRead()
    {
        char buffer[1024];
        ssize_t n = recv(__fd, buffer, sizeof(buffer), 0);
        std::string temp = std::string(buffer);
        std::cout << "[Info] Message recieved from connection " << __fd << ": " + temp << std::endl;
        if (n <= 0) {
            __loop->removeChannel(&__channel);
            delete this;
            return;
        }

        std::string msg(buffer, n);
        __pool.submit([fd = __fd, msg]() {
            send(fd, msg.c_str(), msg.size(), 0);
        });
    }
};

class Acceptor {

private:
    void handleAccept()
    {
        while(true) {
            int connfd = accept(__listenfd, nullptr, nullptr);
            if (connfd < 0) break;
            new Connection(__loop, connfd, __pool);
        }
    }

    EventLoop* __loop;
    int __listenfd;
    ThreadPool& __pool;
    Channel __channel;

public:
    Acceptor(EventLoop* loop, int listenfd, ThreadPool& pool)
        : __loop(loop), __listenfd(listenfd), __pool(pool), __channel(listenfd)
    {
        __channel.setReadCallback([this]() { handleAccept(); });
        __loop->addChannel(&__channel);
    }
};

class TCPServer 
{
private:
    int __listenfd;
    EventLoop __loop;
    ThreadPool __pool;
    std::unique_ptr<Acceptor> acceptor_;

public:
    explicit TCPServer(uint16_t port) : __loop(), __pool(50)
    {
        __listenfd = socket(AF_INET, SOCK_STREAM, 0);
        setNonBlocking(__listenfd);

        int opt = 1;
        setsockopt(__listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if(bind(__listenfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
        {
            perror("Failed to bind socket!");
        }

        int ret = listen(__listenfd, 1024);
        if(ret == -1) perror("Failed to set listening!");

        acceptor_ = std::make_unique<Acceptor>(&__loop, __listenfd, __pool);
    }

    ~TCPServer() noexcept
    {
        if(__listenfd != -1) close(__listenfd);
    }
    void start()
    {
        std::cout << "[INFO] Reactor server started\n";
        __loop.loop();
    }

};

int main()
{
    signal(SIGPIPE, SIG_IGN);

    try {
        TCPServer server(9999);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
