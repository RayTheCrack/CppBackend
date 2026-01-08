// BBL DRIZZY
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <string>
#include <set>
#include <algorithm>

class TCPServer
{
private:
    int max_fd;
    int server_fd;
    fd_set read_fds;
    const uint16_t server_port;
    std::set<int,std::greater<int>> client_fds;
    

    void error(const std::string& msg, bool CloseServer = true)
    {
        std::cerr << "[Error] " << msg << std::endl;
        for(int fd : client_fds) if(fd != -1) close(fd);
        client_fds.clear();
        if(CloseServer and server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        exit(EXIT_FAILURE);
    }

public:

    explicit TCPServer(const uint16_t _port) : server_port(_port), server_fd(-1), max_fd(-1)
    {
        FD_ZERO(&read_fds);
    }

    ~TCPServer() noexcept
    {
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

    void select_loop()
    {   
        std::cout << "[Info] Server is waiting for client connections..." << std::endl;
        fd_set temp_fds;
        while(true)
        {
            temp_fds = read_fds;

            int cnt = select(max_fd + 1, &temp_fds, nullptr, nullptr, nullptr);
            if(cnt == -1)
            {
                error("Failed to call select!", false);
                continue;
            }
            
            for(int fd=0;fd<=max_fd;fd++)
            {
                if(FD_ISSET(fd, &temp_fds))
                {
                    // 此时说明有新的客户端连接
                    if(fd == server_fd)
                    {
                        sockaddr_in client_addr{};
                        socklen_t siz = sizeof(client_addr);

                        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &siz);
                        if(client_fd == -1)
                        {
                            std::cerr << "[Warning] Failed to accept client connection! Continue..." << std::endl;
                            continue;
                        }

                        char client_ip[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip));
                        uint16_t client_port = ntohs(client_addr.sin_port);
                        std::cout << "[Info] New client connected: IP = " << client_ip << ", Port = " << client_port << std::endl;

                        FD_SET(client_fd, &read_fds);
                        client_fds.insert(client_fd);
                        max_fd = std::max(server_fd, *client_fds.begin());

                    }
                    else // 否则则是客户端进行通信
                    {
                        char buffer[1024]{};
                        ssize_t msg_len = recv(fd, buffer, sizeof(buffer) - 1,0);
                        if(msg_len > 0)
                        {
                            buffer[msg_len] = '\0';
                            std::cout << "[Client " << fd << "] message : " << buffer << std::endl;

                            std::string response = "Server received message : " + std::string(buffer);
                            ssize_t resp_len = send(fd, response.c_str(), response.size(), 0);
                            if(resp_len == -1)
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
                            // 从客户端fd容器中移除;
                            client_fds.erase(fd);
                            if(client_fds.empty()) max_fd = server_fd;
                            else max_fd = std::max(server_fd, *client_fds.begin());
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
    server.select_loop();
    std::cout << "Started server sucessfully!" << std::endl;
    return 0;
}