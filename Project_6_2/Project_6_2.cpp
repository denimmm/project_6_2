/**
 * @file Project_6_2.cpp
 * @brief Acts as the Entry point to the client application
 * @author Jax Drummond
 */

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

/**
 * @brief Entry point of the client application. Connects to the server, sends the aircraft ID, and starts the telemetry module.
 * @param argc The number of command line arguments. Should be 4 (program name, IP address, port, aircraft ID).
 * @param argv The command line arguments. argv[1] should be the server IP address, argv[2] should be the port number, and argv[3] should be the aircraft ID.
 * @return Returns 0 on successful shutdown, -1 on error.
 */
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

	cout << "starting telemetry\n";

	telem.Run(client); // Run the telemetry manager

	client.Close();

	cout << "Transmission completed.\n";
}
