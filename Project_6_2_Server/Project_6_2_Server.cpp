// Project_6_2_Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "Packets.h"


#pragma comment(lib, "ws2_32.lib")

#define LOGFOLDER "Logs/"

#define TIMESTAMP_SIZE sizeof(int)
#define FUEL_SIZE sizeof(float)
#define AIRCRAFT_ID_SIZE sizeof(int)

using namespace std;

//fuel per second
float calculateFuelConsumption(int deltaTime, float deltaFuel) {
    return deltaFuel / deltaTime;
}

float calculateAverageFuelConsumption(int packets_received, float average_fuel_consumption, float fuel_consumption) {
    if (packets_received == 0) {
        return fuel_consumption;
    }

    return ((average_fuel_consumption * packets_received) + fuel_consumption) / (packets_received + 1);
}

void handleClient(SOCKET clientSock) {
    char buffer[1024];
    int bytes;

    ofstream file;

    int _aircraftID = -1;
    float _fuel_consumption = 0;
    float _current_fuel = 0;
    float _average_fuel_consumption = 0;
    int _current_time = 0;
    int _packets_received = 0;

    while ((bytes = recv(clientSock, buffer, sizeof(buffer), 0)) > 0) {

        

        switch (bytes) {

        //handle aircraft id packet
        case AIRCRAFT_ID_SIZE:
            
            memcpy(&_aircraftID, buffer, AIRCRAFT_ID_SIZE);
            for (int i = 0; i < 5; i++) {

                if (file) {
                    file.close();
                }

                //attempt to make a new file
                file.open(LOGFOLDER + to_string(_aircraftID) + ".txt");

                //check if file opened, try again if not.
                if (file) {
                    break;
                }

                cerr << "File failed to open for aircraft: " << _aircraftID;
            }

            //if file failed to open, close the connection
            if (!file) {
                closesocket(clientSock);
                file.close();
                return;
            }

            break;

        //handle regular telemetry packet
        case TIMESTAMP_SIZE + FUEL_SIZE:
            int timestamp = 0;
            float fuel_amount = 0;

            //parse
            memcpy(&timestamp, buffer, TIMESTAMP_SIZE);
            memcpy(&fuel_amount, buffer + TIMESTAMP_SIZE, FUEL_SIZE);



            //calculate current fuel consumption
            _fuel_consumption = calculateFuelConsumption(timestamp - _current_time, fuel_amount - _current_fuel);
            
            //calculate average fuel consumption
            _average_fuel_consumption = calculateAverageFuelConsumption(_packets_received, _average_fuel_consumption, _fuel_consumption);

            //set current values
            _current_time = timestamp;
            _current_fuel = fuel_amount;
            _packets_received++;

            //store fuel amount
            file << _fuel_consumption << endl;

            break;


        }


    }

    //store average fuel consumption when transmission ends
    file << "average fuel consumption: " << _average_fuel_consumption << endl;
    closesocket(clientSock);
    std::cout << "Client disconnected\n";
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


