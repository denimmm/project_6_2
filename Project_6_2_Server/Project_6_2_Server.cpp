// Project_6_2_Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>


#pragma comment(lib, "ws2_32.lib")

using namespace std;

void handleClient(SOCKET clientSock) {
    char buffer[1024];
    int bytes;

    while ((bytes = recv(clientSock, buffer, sizeof(buffer), 0)) > 0) {
        std::string msg(buffer, bytes);
        std::cout << "Received: " << msg << std::endl;

    }

    std::cout << "Client disconnected\n";
    closesocket(clientSock);
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }


    if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port 8080...\n";

    std::vector<std::thread> threads;

    while (true) {
        SOCKET clientSock = ::accept(serverSock, nullptr, nullptr); // global namespace
        if (clientSock == INVALID_SOCKET) {
            std::cerr << "Accept failed\n";
            continue;
        }

        std::cout << "Client connected!\n";

        //spawn a new thread to handle this client
        threads.emplace_back(handleClient, clientSock);
        threads.back().detach(); //detach so thread cleans up automatically
    }

    closesocket(serverSock);
    WSACleanup();
}


