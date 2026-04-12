// Project_6_2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <winsock2.h>
#include <fstream>
#include <string>
#include <sstream>
#include <ws2tcpip.h>

#include "TelemetryModule.h"
#include "ClientNetwork.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int NUMBER_OF_ARGS = 4;


int main(int argc, char* argv[]) {

	//check arg count
	if (argc < NUMBER_OF_ARGS) {
		cout << "please enter <IP_Address> <Port> <Aircraft_ID> as commandline arguments \n";
		return 0;
	}

	//get args
	string Server_IP = argv[1];
	string Port = argv[2];
	int ID = atoi(argv[3]);

	ClientNetwork client;

	//try to connect to server
	if (!client.Connect(Server_IP, stoi(Port))) {
		cout << "Connection failed\n";
		return 1;
	}

	srand(time(NULL));

	//Aircraft ID 
	IDPacket idPacket;
	idPacket.aircraftID = ID;

	//Send ID packet
	if (!client.SendIDPacket(idPacket)) {
		std::cout << "Failed to send ID packet\n";
		client.Close();
		return 1;
	}

	TelemetryModule telem; // Constructor initializes

	//try to connect 5 times

	//if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
	//    std::cout << "Connection failed\n";
	//    return 1;
	//}

	cout << "starting telemetry\n";

	telem.Run(client); // Run the telemetry manager


	client.Close();

	cout << "Transmission completed.\n";
}
