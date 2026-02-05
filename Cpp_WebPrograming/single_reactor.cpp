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

int SetNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    return fcntl(fd ,F_SETFL, flag | O_NONBLOCK);
}
// 事件类
class Channel {

    using CB_Func = std::function<void()>;

private:
    int __fd;
    CB_Func __read_cb;

public:

    explicit Channel(int fd) : __fd(fd) {}

    // 在Connection类关闭，在Channel类再关闭会报错
    // ~Channel() noexcept {
    //     if(__fd != -1) 
    //         close(__fd);
    // }

    void SetReadCallBack(CB_Func cb) { __read_cb = std::move(cb); }
    
    void HandleRead(uint32_t event) {
        if((event & EPOLLIN) and __read_cb) {
            __read_cb();
        }
    }

    int fd() const { return __fd; }
};

// Epoll封装 由于监听fd是否存在就绪事件
class EpollEventLoop {

private:
    int __epfd;
    std::atomic<bool> __is_running{true};
    std::vector<epoll_event> __events; 

public:

    explicit EpollEventLoop(int size = 1024) {
        __events.resize(size);
        __epfd = epoll_create1(EPOLL_CLOEXEC);
        if(__epfd == -1)
            throw std::runtime_error("Failed to create epoll!");
    }

    ~EpollEventLoop() noexcept {
        __is_running.store(false);
        if(__epfd != -1)
            close(__epfd);
    }

    void AddChannel(Channel* ch) {
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.ptr = ch;
        epoll_ctl(__epfd, EPOLL_CTL_ADD, ch->fd(), &ev);
    }

    void DelChannel(Channel* ch) {
        epoll_ctl(__epfd, EPOLL_CTL_DEL, ch->fd(), nullptr);
    }

    void loop() {
        while (__is_running) {
            int nfds = epoll_wait(__epfd, __events.data(), __events.size(), -1);
            for(int i=0;i<nfds;i++) {
                auto* ch = static_cast<Channel*>(__events[i].data.ptr);
                ch->HandleRead(__events[i].events);
            }
        }
    }

    void stop() { __is_running.store(false);}
};

// 客户端连接
class Connection {

private:
    int __fd;
    EpollEventLoop* __epoll;
    ThreadPool& __pool;
    Channel __channel;
    
public:
    Connection(EpollEventLoop* epoll, ThreadPool& pool, int fd) :
        __epoll(epoll), __pool(pool), __fd(fd), __channel(fd) {
            SetNonBlocking(__fd);
            __channel.SetReadCallBack([this](){HandleRead();});
            epoll->AddChannel(&__channel);
        };

    ~Connection() noexcept {
        if(__fd != -1)
            close(__fd);
    }

    void HandleRead() 
    {
        char buffer[1024];
        ssize_t len = recv(__fd, buffer, sizeof(buffer), 0);
        if(len <= 0) {
            std::cout << "[Info] Client " << __fd << " disconnected! Resource destoryed!\n";
            __epoll->DelChannel(&__channel);
            delete this;
            return;
        } 
        std::string temp = std::string(buffer,len);
        std::cout << "[Info] Message recieved from client " << __fd << ": " + temp << std::endl;
        __pool.submit([fd = __fd, temp]()
        {
            send(fd, temp.c_str(), temp.size(), 0);
        });
    }
};

class Acceptor {

private:
    int __listenfd;
    EpollEventLoop* __epoll;
    ThreadPool& __pool;
    Channel __channel;

    void HandleAccept() {
        while (true) {
            int client_fd = accept(__listenfd, nullptr, nullptr);
            if(client_fd == -1) break;
            new Connection(__epoll, __pool, client_fd);
        }
    }

public:
    Acceptor(EpollEventLoop* epoll, ThreadPool& pool, int lisfd) :
        __epoll(epoll), __pool(pool), __channel(lisfd), __listenfd(lisfd) {
            __channel.SetReadCallBack([this](){ HandleAccept();});
            __epoll->AddChannel(&__channel);
        } 
};

class TCPServer {

private:
    int __listenfd;
    EpollEventLoop __epoll;
    ThreadPool __pool;
    // 使用智能指针提前声明，因为acceptor需要延迟初始化
    std::unique_ptr<Acceptor> __acceptor;

public:
    TCPServer(uint16_t port) : __pool(50), __epoll() 
    {
        __listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if(__listenfd == -1) {
            perror("Failed to create socket!");
        }

        SetNonBlocking(__listenfd);

        int opt = 1;
        setsockopt(__listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if(bind(__listenfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
            perror("Failed to bind socket!");
        }

        int ret = listen(__listenfd, 128);
        if(ret == -1) perror("Failed to set listening!");

        __acceptor = std::make_unique<Acceptor>(&__epoll, __pool, __listenfd);
    }

    ~TCPServer() noexcept {
        if(__listenfd != -1)
            close(__listenfd);
        __epoll.stop();
    }

    void start()
    {
        std::cout << "[Info] Reactor server started!\n";
        __epoll.loop();
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
