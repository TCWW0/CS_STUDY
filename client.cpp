#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define BUFFER_SIZE 4096
#define DEFAULT_PATH "/home.html"  // 默认请求路径

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip_or_hostname> <port>" << std::endl;
        return -1;
    }

    const char* server_ip = argv[1];  // 服务器 IP 或域名
    int server_port = std::stoi(argv[2]);  // 端口号

    // 创建 socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Socket creation failed!" << std::endl;
        return -1;
    }

    // 设置服务器地址
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    // 如果传入的是域名，先解析成 IP 地址
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        struct hostent *host = gethostbyname(server_ip);
        if (host == nullptr) {
            std::cerr << "Unable to resolve host: " << server_ip << std::endl;
            close(sockfd);
            return -1;
        }
        server_addr.sin_addr = *(struct in_addr*)host->h_addr_list[0];
    }

    // 连接服务器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection to server failed!" << std::endl;
        close(sockfd);
        return -1;
    }

    // 向服务器发送 HTTP 请求
    std::string request = "GET " + std::string(DEFAULT_PATH) + " HTTP/1.0\r\nHost: " + server_ip + "\r\n\r\n";
    send(sockfd, request.c_str(), request.size(), 0);

    // 接收并输出服务器响应
    char buffer[BUFFER_SIZE];
    int bytes_received;
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        std::cout << buffer;
    }

    // 让用户输入数据并发送给服务器
    std::string user_input;
    std::cout << "\nEnter a line of text to send to the server: ";
    std::getline(std::cin, user_input);

    // 发送用户输入数据到服务器
    send(sockfd, user_input.c_str(), user_input.size(), 0);

    // 接收并输出服务器返回的数据
    bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::cout << "Server responded: " << buffer << std::endl;
    }

    // 关闭连接
    close(sockfd);
    return 0;
}


