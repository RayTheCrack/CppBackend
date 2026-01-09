#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

class TCPClient
{
private:
    int client_fd;
    std::string server_ip;
    uint16_t port;

    void error(const std::string& msg, bool ClientClose = true)
    {
        std::cerr << "[Error] " << msg << std::endl;
        if(ClientClose and client_fd != -1)
        {
            close(client_fd);
            client_fd = -1;
        }
        exit(EXIT_FAILURE);
    }

public:
    // 构造函数：初始化服务器地址和端口，套接字置为无效值
    TCPClient(const std::string& _server_ip, uint16_t server_port) : 
    server_ip(_server_ip), port(server_port), client_fd(-1) {};
    
    ~TCPClient() noexcept
    {
        if(client_fd != -1) close(client_fd);
    }
    // 初始化客户端， 创建套接字 -> 连接服务器
    void connectServer()
    {
        client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(client_fd == -1)
        {
            error("Failed to create a socket!");
        }
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        if(inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr.s_addr) <= 0)
        {
            error("Invalid IP : " + server_ip);
        }
        std::cout << "Connecting to the server :" << server_ip << "...\n";
        int ret = connect(client_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
        if(ret == -1)
        {
            error("Failed to connect server!");
        }
        std::cout << "Successfully connected to the server!" << std::endl;
    }

    void communicate()
    {
        char recv_buffer[1024]{};
        std::string msg;
        while(true)
        {
            std::cout << "Enter the message you want to send (empty to exit) : ";
            getline(std::cin, msg);
            if(msg.size() == 0)
            {
                std::cout << "Exited successfully!" << std::endl;
                break;
            }

            ssize_t send_len = send(client_fd, msg.c_str(), msg.size(), 0);
            if(send_len == -1)
            {
                error("Failed to send messages to server!", false);
                break;
            }

            ssize_t recv_len = recv(client_fd, recv_buffer, sizeof(recv_buffer) - 1, 0);
            if(recv_len > 0)
            {
                recv_buffer[recv_len] = '\0';
                std::cout << "[Info] Received message from server : " << recv_buffer << std::endl;
            }
            else if(recv_len == 0)
            {
                std::cout << "Server disconnected!" << std::endl;
                break;
            }
            else 
            {
                error("Failed to receive data from server!", false);
                break;
            }

            memset(recv_buffer, 0, sizeof(recv_buffer));
        }
    }   
};

int main()
{
    TCPClient client("192.168.182.128", 9999);
    client.connectServer();
    client.communicate();
    std::cout << "Client shutdown successfully!" << std::endl;
    return 0;
}
/*
# 测试参数说明：
# -c 1000：并发连接数1000
# -t 8：使用8个线程
# -d 30s：测试时长30秒
wrk -c 1000 -t 8 -d 30s -s test.lua tcp://192.168.182.128:9999
*/