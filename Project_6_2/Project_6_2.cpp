// Project_6_2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <winsock2.h>
#include <fstream>
#include <string>
#include <sstream>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int NUMBER_OF_ARGS = 4;
const string TELEMETRY_FILE_NAME = "test.txt";


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

    //choose a file to open
    fstream telemetry_file(TELEMETRY_FILE_NAME);

    if (!telemetry_file.is_open()) {
        cout << "Failed to open file :C\n";
    }

    //try to connect 5 times

    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cout << "Connection failed\n";
        return 1;
    }

    bool flying = true;

    //send data

    while (flying) {
        string line;
        while (getline(telemetry_file, line)) {
            cout << "line\n";

            //parse
            stringstream ss(line);
            string timestamp, current_fuel;
            getline(ss, timestamp, ',');
            getline(ss, current_fuel, ',');

            //send packet
            int bytesSent = send(sock, line.c_str(), line.length(), 0);

            if (bytesSent == SOCKET_ERROR) {
                std::cout << "Send failed\n";
            }
            else {
                std::cout << "Sent " << bytesSent << " bytes\n";
            }
        }
    }


    closesocket(sock);
    WSACleanup();
    telemetry_file.close();

    cout << "Transmission completed.\n";
}
