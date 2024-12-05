#include <iostream>
#include <string>
#include <cstring>
#include <WinSock2.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define MAX_BUFFER 4096

void error(const std::string& msg) {
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}

void handle_client(SOCKET client_socket) {
    char buffer[MAX_BUFFER] = {0};

    int bytes_received = recv(client_socket, buffer, MAX_BUFFER - 1, 0);
    if (bytes_received <= 0) {
        std::cerr << "Failed to read request" << std::endl;
        closesocket(client_socket);
        return;
    }

    std::cout << "Client Request:\n" << buffer << std::endl;

    std::string request(buffer);
    size_t host_pos = request.find("Host: ");
    if (host_pos == std::string::npos) {
        std::cerr << "No host header found in request" << std::endl;
        closesocket(client_socket);
        return;
    }

    host_pos += 6;
    size_t host_end = request.find("\r\n", host_pos);
    std::string host = request.substr(host_pos, host_end - host_pos);

    std::cout << "Extracted Host: " << host << std::endl;

    struct hostent* server = gethostbyname(host.c_str());
    if(!server) {
        std::cerr << "Host not found: " << host << std::endl;
        closesocket(client_socket);
        return;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create server socket" << std::endl;
        closesocket(client_socket);
        return;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    closesocket(client_socket);
    closesocket(server_socket);
    return;
    }

    send(server_socket, buffer, bytes_received, 0);

    char response[MAX_BUFFER];
    int bytes;
    while ((bytes = recv(server_socket, response, MAX_BUFFER - 1, 0)) > 0) {
        send(client_socket, response, bytes, 0);
    }

    closesocket(server_socket);
    closesocket(client_socket);

}

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);

    if(WSAStartup(MAKEWORD(2, 3), &wsa) != 0) {
        error("Failed to initialize winsock");
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) { 
        error("Could not create socket");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        error("Bind Failed");
    }

    if (listen(server_socket, 3) == SOCKET_ERROR) {
        error("Listen Failed");
    }

    std::cout << "Proxy server listening on port" << PORT << std::endl;

    while(true) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }

        std::cout << "Client connected" << std::endl;

        std::thread(handle_client, client_socket).detach();
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}