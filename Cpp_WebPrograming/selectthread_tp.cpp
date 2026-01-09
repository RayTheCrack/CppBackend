#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <string>
#include <set>
#include <algorithm>
#include <thread>
#include <cerrno>
#include <stdexcept>
#include <signal.h>
#include <mutex>
#include <atomic>
#include <vector>
#include <condition_variable>
#include <memory>
#include <utility>
#include <queue>
#include <functional>

class ThreadPool
{

private:

    std::mutex mtx;
    std::queue<std::function<void()>> task_queue;
    std::vector<std::thread> threads;
    std::atomic<bool> stop{false}; 
    std::condition_variable cv;
    
    void NewThread()
    {
        while(true)
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock,[this](){return !task_queue.empty() or stop.load();});
            if(stop.load() and task_queue.empty())
                break;
            if(task_queue.empty())
                continue;
            std::function<void()> task = std::move(task_queue.front());
            task_queue.pop();
            lock.unlock();
            task();
            if(task_count.fetch_sub(1) == 1)
            {
                std::unique_lock<std::mutex> endlock(end_mtx);
                end_cv.notify_one();
            }
        }
    }

public:

    std::atomic<int> task_count{0};
    std::condition_variable end_cv;
    std::mutex end_mtx;

    explicit ThreadPool(int ThreadsCnt) : stop(false)
    {
        for(int i=1;i<=ThreadsCnt;i++) 
        {
            threads.emplace_back(&ThreadPool::NewThread,this);
        }
    };  

    ThreadPool(const ThreadPool& other) = delete;
    ThreadPool& operator=(const ThreadPool& other) = delete;

    ThreadPool(ThreadPool&& other) noexcept = default;
    ThreadPool& operator=(ThreadPool&& other) noexcept = default;

    ~ThreadPool() noexcept
    {
        {
            std::unique_lock<std::mutex> lock(mtx);
            stop.store(true);
        }
        cv.notify_all();
        for(auto &t : threads)
        {
            if(t.joinable())
                t.join();
        }
    }

    template<typename F,typename...Args>
    void submit(F&& f,Args&& ...args)
    {
        if(stop.load() == true) 
        {
            std::cerr << "The ThreadPool is already stopped!" << std::endl;
            return;
        }
        std::function<void()> task = std::bind(std::forward<F>(f),std::forward<Args>(args)...);
        {
            std::unique_lock<std::mutex> lock(mtx);
            task_queue.emplace(std::move(task));
            task_count.fetch_add(1);
        }
        cv.notify_one();
    } 
};

class TCPServer
{
private:
    int max_fd;
    int server_fd;
    fd_set read_fds;
    std::atomic<bool> is_running;
    const uint16_t server_port;
    std::set<int,std::greater<int>> client_fds;
    std::mutex mtx;
    ThreadPool pool;

    void error(const std::string& msg, bool CloseServer = true)
    {
        std::unique_lock<std::mutex> lock(mtx);
        std::cerr << "[Error] " << msg << std::endl;
        for(int fd : client_fds) if(fd != -1) close(fd);
        client_fds.clear();
        if(CloseServer and server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        is_running = false;
        throw std::runtime_error(msg);
    }

public:

    explicit TCPServer(const uint16_t _port) : server_port(_port), server_fd(-1), max_fd(-1), is_running(true), pool(10)
    {
        FD_ZERO(&read_fds);
    }

    ~TCPServer() noexcept
    {
        stop();
        std::unique_lock<std::mutex> lock(mtx);
        for(int fd : client_fds) if(fd != -1) close(fd);
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

        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;

        std::cout << "Server is currently listening on port: " << server_port << std::endl; 
    }

    void handle_client_io(int fd)
    {
        char buffer[1024]{};
        ssize_t len = recv(fd, buffer, sizeof(buffer), 0);

        if(len <= 0)
        {
            std::cout << "[Info] Client " << fd << " disconnected\n";
            // 加锁修改共享资源
            std::unique_lock<std::mutex> lock(mtx);
            close(fd);
            FD_CLR(fd, &read_fds);
            client_fds.erase(fd);
            if(client_fds.empty()) max_fd = server_fd;
            else max_fd = std::max(server_fd, *client_fds.begin());
            return;
        }
        std::string msg(buffer,len);
        pool.submit([this, fd, msg]()
        {
            std::string resp = "Server received: " + msg;
            // 发送时无需锁（每个 fd 对应一个客户端，send 是原子操作），但需检查 is_running
            if(is_running) send(fd, resp.c_str(), resp.size(), 0);
        });
    }
    void accept_client()
    {
        int cfd = accept(server_fd, nullptr, nullptr);
        if(cfd < 0) return;
        // 加锁修改共享资源
        std::unique_lock<std::mutex> lock(mtx);
        FD_SET(cfd, &read_fds);
        client_fds.insert(cfd);
        max_fd = std::max(max_fd, cfd);

        std::cout << "[Info] New client fd = " << cfd << std::endl;
    }

    void select_loop()
    {
        while(is_running)
        {
            fd_set tmp;
            int current_max_fd;
            // 加锁拷贝共享的 read_fds 和 max_fd
            {
                std::unique_lock<std::mutex> lock(mtx);
                tmp = read_fds;
                current_max_fd = max_fd;
            }

            int ret = select(current_max_fd + 1, &tmp, nullptr, nullptr, nullptr);
            if(ret < 0) 
            {
                if(errno == EINTR) continue; // 处理信号中断
                break;
            }

            // 遍历 fd 时无需锁（操作的是拷贝后的 tmp）
            for(int fd=0;fd<=current_max_fd;fd++)
            {
                if(!FD_ISSET(fd, &tmp)) continue;
                if(fd == server_fd)
                {
                    accept_client(); // 内部已加锁
                }
                else
                {
                    handle_client_io(fd); // 内部已加锁
                }
            }
        }
    }

    void stop()
    {
        is_running = false;
        if(server_fd != -1)
            shutdown(server_fd, SHUT_RD);
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
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signals);
    TCPServer server(9999);
    ptr = &server;

    try {
        server.init();
        server.select_loop();}
    catch(const std::runtime_error& e) {
        std::cout << e.what() << std::endl;
    }

    std::cout << "Closed server sucessfully!" << std::endl;
    return 0;
}