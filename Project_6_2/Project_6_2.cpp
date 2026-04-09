// Project_6_2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <winsock2.h>
#include <fstream>
#include <string>
#include <sstream>
#include <ws2tcpip.h>
#include "TelemetryModule.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int NUMBER_OF_ARGS = 4;


int main(int argc, char* argv[]) {

    //check arg count
    if(argc < NUMBER_OF_ARGS){
        cout << "please enter <IP_Address> <Port> <Aircraft_ID> as commandline arguments \n";
        return 0;
    }

    //get args
    string Server_IP = argv[1];
    string Port = argv[2];
    string Aircraft_ID = argv[2];

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(stoi(Port));
    inet_pton(AF_INET, Server_IP.c_str(), &server.sin_addr);
    
    TelemetryModule telem; // Constructor initializes

    //try to connect 5 times

    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cout << "Connection failed\n";
        return 1;
    }

    telem.Run(); // Run the telemetry manager


    closesocket(sock);
    WSACleanup();

    cout << "Transmission completed.\n";
}
