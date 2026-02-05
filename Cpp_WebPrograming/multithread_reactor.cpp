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
#include <thread>
#include <algorithm>

#include "threadpool.h"

int SetNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    return fcntl(fd ,F_SETFL, flag | O_NONBLOCK);
}

// 事件类（完全保留原有逻辑，无修改）
class Channel {
    using CB_Func = std::function<void()>;
private:
    int __fd;
    CB_Func __read_cb;
public:
    explicit Channel(int fd) : __fd(fd) {}
    void SetReadCallBack(CB_Func cb) { __read_cb = std::move(cb); }
    void HandleRead(uint32_t event) {
        if((event & EPOLLIN) and __read_cb) {
            __read_cb();
        }
    }
    int fd() const { return __fd; }
};

// Epoll封装
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
    // 新增：返回当前EventLoop是否在运行（供从Reactor线程池使用）
    bool isRunning() const { return __is_running; }
};

// 从Reactor线程池（管理多个从Reactor，负责分发connfd）
class ReactorThreadPool {
private:
    std::vector<std::unique_ptr<EpollEventLoop>> __sub_reactors;
    std::vector<std::thread> __reactor_threads;
    std::atomic<int> __next_reactor{0}; // 轮询分发索引
public:
    // 初始化从Reactor线程池
    void init(int sub_reactor_num, ThreadPool& work_pool) {
        if (sub_reactor_num <= 0) sub_reactor_num = 1;
        for (int i = 0; i < sub_reactor_num; ++i) {
            // 创建从Reactor
            auto sub_reactor = std::make_unique<EpollEventLoop>();
            __sub_reactors.emplace_back(std::move(sub_reactor));
            // 启动从Reactor的事件循环线程
            __reactor_threads.emplace_back([this, i, &work_pool]() {
                __sub_reactors[i]->loop();
            });
        }
    }

    // 轮询获取一个从Reactor（分发connfd使用）
    EpollEventLoop* getNextSubReactor() {
        if (__sub_reactors.empty()) return nullptr;
        int idx = __next_reactor.fetch_add(1) % __sub_reactors.size();
        return __sub_reactors[idx].get();
    }

    // 停止所有从Reactor
    ~ReactorThreadPool() noexcept {
        for (auto& sub_reactor : __sub_reactors) {
            sub_reactor->stop();
        }
        for (auto& thread : __reactor_threads) {
            if (thread.joinable()) thread.join();
        }
    }
};

// 客户端连接（仅修改：构造函数接收从Reactor，保留原有逻辑）
class Connection {
private:
    int __fd;
    EpollEventLoop* __epoll; // 现在指向从Reactor
    ThreadPool& __pool;
    Channel __channel;
public:
    // Reactor参数改为从Reactor（由主Reactor分发而来）
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

// Acceptor（获取从Reactor，分发connfd，保留原有连接逻辑）
class Acceptor {
private:
    int __listenfd;
    EpollEventLoop* __main_reactor; // 主Reactor（仅处理连接）
    ReactorThreadPool& __sub_reactor_pool; // 从Reactor线程池
    ThreadPool& __work_pool; // 业务工作池
    Channel __channel;

    void HandleAccept() {
        while (true) {
            int client_fd = accept(__listenfd, nullptr, nullptr);
            if(client_fd == -1) break;
            // 核心修改：获取一个从Reactor，将connfd交给从Reactor处理
            EpollEventLoop* sub_reactor = __sub_reactor_pool.getNextSubReactor();
            if (sub_reactor) {
                new Connection(sub_reactor, __work_pool, client_fd);
            } else {
                close(client_fd); // 无可用从Reactor，关闭连接
            }
        }
    }

public:
    // 构造函数修改：新增从Reactor线程池参数
    Acceptor(EpollEventLoop* main_reactor, ReactorThreadPool& sub_reactor_pool, ThreadPool& work_pool, int lisfd) :
        __main_reactor(main_reactor), __sub_reactor_pool(sub_reactor_pool), __work_pool(work_pool), 
        __channel(lisfd), __listenfd(lisfd) {
            __channel.SetReadCallBack([this](){ HandleAccept();});
            __main_reactor->AddChannel(&__channel);
        } 
};

// TCPServer（新增从Reactor线程池，主Reactor仅处理连接）
class TCPServer {
private:
    int __listenfd;
    EpollEventLoop __main_reactor; // 主Reactor（仅处理客户端连接）
    ReactorThreadPool __sub_reactor_pool; // 从Reactor线程池（处理客户端IO）
    ThreadPool __work_pool; // 原有业务工作池（保留）
    std::unique_ptr<Acceptor> __acceptor;

public:
    TCPServer(uint16_t port) : __work_pool(50), __main_reactor() 
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

        // 初始化从Reactor线程池（这里设置4个从Reactor，可调整）
        __sub_reactor_pool.init(4, __work_pool);

        // 初始化Acceptor（传入主Reactor和从Reactor线程池）
        __acceptor = std::make_unique<Acceptor>(&__main_reactor, __sub_reactor_pool, __work_pool, __listenfd);
    }

    ~TCPServer() noexcept {
        if(__listenfd != -1)
            close(__listenfd);
        __main_reactor.stop();
    }

    void start()
    {
        std::cout << "[Info] Master-Slave Multi-Thread Reactor server started!\n";
        __main_reactor.loop(); // 主Reactor启动事件循环（仅处理连接）
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