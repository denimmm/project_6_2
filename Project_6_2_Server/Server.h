#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <filesystem>
#include <unordered_map>

#include "Packets.h"

using namespace std;

typedef struct Task {
	int AircraftID;
	DataPacket data;
};

typedef struct Client {
	SOCKET socket;
	int ID;
	int packets_received;
	int current_time;
	float current_fuel;
	float current_fuel_consumption;
	float average_fuel_consumption;

};

class Server {
	SOCKET serverSock;
	int Port;


	//maps for tracking client structs and their files.
	unordered_map<int, unique_ptr<Client>> ClientMap;
	unordered_map<int, ofstream> FileMap;

	//queue for tracking tasks. tasks are created and added when a packet is received. worker threads handle tasks in the queue
	queue<Task> TaskQueue;

	Server();


	
	void OnConnect();
	void OnDisconnect();
	void TelemetryHandler();
	void ConnectionHandler();
	void SocketHandler();

	DataPacket recvAll(Client client, int NumberOfBytes);
	float calculateFuelConsumption(Client client);

	float calculateAverageFuelConsumption(Client client);

};