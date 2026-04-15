
#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include "Packets.h"


#pragma comment(lib, "ws2_32.lib")

#define LOGFOLDER "Logs/"

#define TIMESTAMP_SIZE sizeof(int)
#define FUEL_SIZE sizeof(float)
#define AIRCRAFT_ID_SIZE sizeof(int)




using namespace std;

typedef struct Client {
	SOCKET clientsock;
	float current_fuel;
	float current_fuel_consumption;
	float average_fuel_consumption;
	int timestamp;
	int packets_received = 0;
	int ID;
	mutex ClientMutex;
	fstream file;

	//constructor
	Client(SOCKET clientsock, int ID) {
		this->clientsock = clientsock;
		this->ID = ID;
		current_fuel = 0;
		current_fuel_consumption = 0;
		average_fuel_consumption = 0;
		timestamp = 0;
		file.open("Logs/" + to_string(ID) + ".txt", ios::app);
	}

};

typedef struct Task {
	int aircraftID;
	char data[TIMESTAMP_SIZE + FUEL_SIZE];
	
	Task(int id, char* buffer) {
		aircraftID = id;
		memcpy(this->data, buffer, TIMESTAMP_SIZE + FUEL_SIZE);
	}
};

class Server {
public:
	SOCKET serverSock;
	bool running;
	vector<thread> ThreadPool;
	unordered_map<int, Client*> ClientMap;
	shared_mutex ClientMapMutex;
	queue<Task> TaskQueue;
	mutex QueueMutex;
	condition_variable TaskCV;
	int port;
	int threads;


	Server(int port, int threads) {
		this->port = port;
		this->threads = threads;
	}

	int Initialize() {
		filesystem::create_directories(LOGFOLDER);

		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
			std::cerr << "WSAStartup failed\n";
			return 1;
		}

		serverSock = socket(AF_INET, SOCK_STREAM, 0);
		if (serverSock == INVALID_SOCKET) {
			std::cerr << "Socket creation failed\n";
			WSACleanup();
			return 1;
		}

		sockaddr_in server{};
		server.sin_family = AF_INET;
		server.sin_port = htons(port);
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

		running = true;
		std::cout << "Server listening on port 8080...\n";

		//spawn worker threads
		for (int i = 0; i < threads -2; i++) { //subtract 2: 1 for main (accepts new connections) and 1 for HandleReceive thread.
			ThreadPool.emplace_back(&Server::HandleTasks, this);
		}

		//spawn receiver thread
		ThreadPool.emplace_back(&Server::HandleReceive, this);

		//loop forever. only accepts connections and recvs 1 packet to get the aircraft id
		while (running) {
			SOCKET clientSock = ::accept(serverSock, nullptr, nullptr); // global namespace
			if (clientSock == INVALID_SOCKET) {
				std::cerr << "Accept failed\n";
				continue;
			}

			std::cout << "Client connected!\n";

			//get the client's ID
			int aircraftID = GetClientID(clientSock);

			//check if aircraft id is invalid or already exists.
			if (aircraftID < 0 || ClientMap.find(aircraftID) != ClientMap.end()) {
				cout << "invalid aircraft ID : " << aircraftID << "disconnecting.\n";
				//close the socket and continue
				closesocket(clientSock);
				continue;
			}
			unique_lock<shared_mutex> lock(ClientMapMutex);
			ClientMap[aircraftID] = new Client(clientSock, aircraftID);
			//std::thread(handleClient, clientSock).detach();
		}
	}

	//deconstruct server
	~Server() {
		//close all sockets and delete all clients
		for (auto& [id, client] : ClientMap) {
			if (client) {
				closesocket(client->clientsock);
				delete client;
			}

		}

		//join all threads
		for (auto& t : ThreadPool) {
			t.join();
		}


		//clean up server
		closesocket(serverSock);
		WSACleanup();
	}
private:
	bool recvAll(SOCKET sock, char* buffer, int totalBytes)
	{
		int bytesReceived = 0;

		while (bytesReceived < totalBytes)
		{
			int result = recv(sock, buffer + bytesReceived, totalBytes - bytesReceived, 0);

			if (result == 0)
			{
				//client disconnected
				return false;
			}

			if (result == SOCKET_ERROR)
			{
				return false;
			}

			bytesReceived += result;
		}

		return true;
	}

	//fuel per second
	float calculateFuelConsumption(int deltaTime, float deltaFuel) {
		return -deltaFuel / deltaTime;
	}

	float calculateAverageFuelConsumption(int packets_received, float average_fuel_consumption, float fuel_consumption) {
		if (packets_received == 0) {
			return fuel_consumption;
		}

		return ((average_fuel_consumption * packets_received) + fuel_consumption) / (packets_received + 1);
	}

	int GetClientID(SOCKET clientSock) {

		char buffer[AIRCRAFT_ID_SIZE] = { 0 };
		int aircraftID = -1;
		if (!recvAll(clientSock, buffer, AIRCRAFT_ID_SIZE)) {
			cout << "Failed to retrieve aircraft ID.\n";
			return -1;
		}

		memcpy(&aircraftID, buffer, AIRCRAFT_ID_SIZE);

		cout << "Aircraft (" << aircraftID << ") connected.\n";

		return aircraftID;
	}

	//
	void HandleTasks() {
		//reusable file pointer
		fstream file;
		while (running) {

			unique_lock<mutex> Qlock(QueueMutex);
			TaskCV.wait(Qlock, [this] {
				return !TaskQueue.empty() || !running;
				});

			//lock mutex, get front task, remove it, unlock mutex
			if (TaskQueue.empty()) {
				Qlock.unlock();
				continue;
			}

			if (!running)
				return;

			Task current_task = TaskQueue.front();
			TaskQueue.pop();
			Qlock.unlock();
	
			//get the aircraft
			shared_lock<shared_mutex> lock(ClientMapMutex);
			auto it = ClientMap.find(current_task.aircraftID);

			//aircraft no longer exists, disconnected, drop the task.
			if (it == ClientMap.end())
				continue;

			Client* current_client = it->second;

			//locks the client object
			lock.unlock();//unlocks previous shared lock
			current_client->ClientMutex.lock();


			//open file
			//file.open("Logs/" + to_string(current_client->ID) + ".txt", ios::app);

			if (!current_client->file.is_open()) {
				current_client->ClientMutex.unlock();
				cout << "ERROR: FILE FAILED TO OPEN: " << current_client->ID << '\n';

				current_client->file.open("Logs/" + to_string(current_client->ID) + ".txt", ios::app);
				//put the task back to try again later
				QueueMutex.lock();
				TaskQueue.push(current_task);
				QueueMutex.unlock();
				continue;
			}

			//process the task
			int timestamp = 0;
			float current_fuel = 0;

			//parse
			memcpy(&timestamp, current_task.data, TIMESTAMP_SIZE);
			memcpy(&current_fuel, current_task.data + TIMESTAMP_SIZE, FUEL_SIZE);

			//calculate current fuel consumption
			current_client->current_fuel_consumption = calculateFuelConsumption(timestamp - current_client->timestamp, current_fuel - current_client->current_fuel);

			//calculate average fuel consumption
			current_client->average_fuel_consumption = calculateAverageFuelConsumption(current_client->packets_received, current_client->average_fuel_consumption, current_client->current_fuel_consumption);

			//increase packet counter
			current_client->packets_received++;

			current_client->current_fuel = current_fuel;
			current_client->timestamp = timestamp;


			//store fuel consumption in fuel/second
			current_client->file << current_client->current_fuel_consumption << '\n';

			current_client->ClientMutex.unlock();
		}

	}

	void OnClientDisconnect(int clientID) {
		unique_lock<shared_mutex> lock(ClientMapMutex);
		auto it = ClientMap.find(clientID);

		//aircraft has already been removed.
		if (it == ClientMap.end())
			return;
		Client* current_client = it->second;

		//save average fuel consumption
		current_client->ClientMutex.lock();
		
		//current_client->file.open("Logs/" + to_string(current_client->ID) + ".txt", ios::app);
		current_client->file << "Average Fuel Consumption: " + to_string(current_client->average_fuel_consumption);
		current_client->file.close();

		//close the client socket
		closesocket(current_client->clientsock);

		//remove the client
		ClientMap.erase(current_client->ID);
		lock.unlock();

		delete current_client;

		cout << "Client disconnected.\n";
	}

	void HandleReceive() {
		while (running)
		{
			std::vector<WSAPOLLFD> fds;
			std::vector<int> idMap; // maps fds[i] -> client ID

			fds.reserve(ClientMap.size());
			idMap.reserve(ClientMap.size());

			// 1. Build poll list (snapshot of current clients)
			for (auto& [id, client] : ClientMap)
			{
				WSAPOLLFD fd{};
				fd.fd = client->clientsock;
				fd.events = POLLRDNORM;

				fds.push_back(fd);
				idMap.push_back(id);
			}

			if (fds.empty())
			{
				Sleep(1);
				continue;
			}

			// 2. Wait for events
			int ret = WSAPoll(fds.data(), (ULONG)fds.size(), -1);

			if (ret <= 0)
				continue;

			// 3. Process events
			for (size_t i = 0; i < fds.size(); i++)
			{
				short re = fds[i].revents;
				int clientId = idMap[i];

				// 3A. Handle socket errors / disconnect signals
				if (re & (POLLERR | POLLHUP | POLLNVAL))
				{
					OnClientDisconnect(clientId);
					continue;
				}

				// 3B. Handle readable socket
				if (re & POLLRDNORM)
				{
					char buffer[TIMESTAMP_SIZE + FUEL_SIZE];

					bool success = recvAll(fds[i].fd, buffer, sizeof(buffer));

					//client disconnected
					if (!success)
					{
						OnClientDisconnect(clientId);
						continue;
					}

					//construct the task
					Task current_task(clientId, buffer);

					QueueMutex.lock();
					TaskQueue.push(current_task);
					QueueMutex.unlock();
					TaskCV.notify_one();

				}
			}
		}
	}
};

//void handleClient(SOCKET clientSock) {
//	char buffer[1024];
//	int bytes;
//
//	ofstream file;
//
//	int _aircraftID = -1;
//	float _fuel_consumption = 0;
//	float _current_fuel = 0;
//	float _average_fuel_consumption = 0;
//	int _current_time = 0;
//	int _packets_received = 0;
//
//	int tries = 0;
//
//	for (int i = 0; i < 5; i++) {
//
//		//attempt to make a new file
//		file.open(LOGFOLDER + to_string(_aircraftID) + ".txt");
//
//		//check if file opened, try again if not.
//		if (file) {
//			break;
//		}
//
//		cerr << "File failed to open file for aircraft: " << _aircraftID << endl;
//	}
//
//	//if file failed to open, close the connection
//	if (!file) {
//		closesocket(clientSock);
//		file.close();
//		cout << "Failed to open file for aircraft (" << _aircraftID << ")\n";
//		return;
//	}
//
//	//handle telemetry
//	while (bool success = recvAll(clientSock, buffer, FUEL_SIZE + TIMESTAMP_SIZE)) {
//
//		if (!success) {
//			break;
//		}
//
//		int timestamp = 0;
//		float fuel_amount = 0;
//
//		//parse
//		memcpy(&timestamp, buffer, TIMESTAMP_SIZE);
//		memcpy(&fuel_amount, buffer + TIMESTAMP_SIZE, FUEL_SIZE);
//
//		//		cout << "Aircraft (" << _aircraftID << ") Fuel: " << fuel_amount;
//		//		cout << "Aircraft (" << _aircraftID << ") timestamp: " << timestamp << "\n";
//
//
//				//calculate current fuel consumption
//		_fuel_consumption = calculateFuelConsumption(timestamp - _current_time, fuel_amount - _current_fuel);
//
//		//calculate average fuel consumption
//		_average_fuel_consumption = calculateAverageFuelConsumption(_packets_received, _average_fuel_consumption, _fuel_consumption);
//
//		//set current values
//		_current_time = timestamp;
//		_current_fuel = fuel_amount;
//		_packets_received++;
//
//		//store fuel amount
//		file << _fuel_consumption << endl;
//
//
//	}
//
//	//store average fuel consumption when transmission ends
//	file << "average fuel consumption: " << _average_fuel_consumption << endl;
//	file.close();
//	closesocket(clientSock);
//	std::cout << "Client disconnected\n";
//}



int main() {


	Server server(8080, 12);

	server.Initialize();
	
	//server deconstructed when out of scope
}


