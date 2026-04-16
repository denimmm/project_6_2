/**
 * @file main.cpp
 * @brief Defines the Server Class and starts program
 * @author Denim MacDougall
 */

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
/**
 * @struct Client
 * @brief Stores data for each client including socket and mutex
 */
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
		file.open(LOGFOLDER + to_string(ID) + ".txt", ios::app);
	}

};

/**
 * @struct Task
 * @brief Stores data and aircraft ID of one task which must be processed by worker threads
 */
typedef struct Task {
	int aircraftID;
	char data[TIMESTAMP_SIZE + FUEL_SIZE];
	
	//constructor
	Task(int id, char* buffer) {
		aircraftID = id;
		memcpy(this->data, buffer, TIMESTAMP_SIZE + FUEL_SIZE);
	}
};

/**
 * @class Server
 * @brief Defines the server class. Performs all operations of the server.
 */
class Server {
public:
	SOCKET serverSock;//server socket
	vector<thread> ThreadPool;//holds all active threads except main
	unordered_map<int, Client*> ClientMap;//holds all client structs
	shared_mutex ClientMapMutex;//shared mutex for ClientMap
	queue<Task> TaskQueue;//holds all active tasks.
	mutex QueueMutex;//mutex for TaskQueue
	condition_variable TaskCV;//condition variable for all threads. wakes up threads when new task is pushed to queue

	bool running;//server shuts down when false
	int port; //server port
	int threads; //total number of threads to start

	/**
	 * @brief Server object constructor
	 *
	 * @param a port of the server
	 * @param b number of threads to start
	 * @return Server object
	 */
	Server(int port, int threads) {
		this->port = port;
		this->threads = threads;
	}

	/**
	 * @brief Starts the server
	 *
	 * @return loops until server shutsdown or fails. returns 1 for failure to start
	 */
	int Initialize() {
		//create directory for logs if it doesnt already exist:
		filesystem::create_directories(LOGFOLDER);

		//start setting up WSA
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

		//bind server socket
		if (bind(serverSock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
			std::cerr << "Bind failed\n";
			closesocket(serverSock);
			WSACleanup();
			return 1;
		}

		//listen on server socket
		if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR) {
			std::cerr << "Listen failed\n";
			closesocket(serverSock);
			WSACleanup();
			return 1;
		}

		running = true;
		std::cout << "Server listening on port 8080...\n";

		//spawn worker threads
		for (int i = 0; i < threads -2; i++) { 
			ThreadPool.emplace_back(&Server::HandleTasks, this);
		}

		//spawn receiver thread
		ThreadPool.emplace_back(&Server::HandleReceive, this);

		//loop forever. only accepts connections and recvs 1 packet to get the aircraft id
		while (running) {
			SOCKET clientSock = ::accept(serverSock, nullptr, nullptr);
			if (clientSock == INVALID_SOCKET) {
				std::cerr << "Accept failed\n";
				continue;
			}

			std::cout << "Client connected!\n";

			//get the client's ID
			int aircraftID = GetClientID(clientSock);

			//check if aircraft id is invalid or already exists. close connection.
			if (aircraftID < 0 || ClientMap.find(aircraftID) != ClientMap.end()) {
				cout << "invalid aircraft ID : " << aircraftID << "disconnecting.\n";
				//close the socket and continue
				closesocket(clientSock);
				continue;
			}

			//add client to client map
			unique_lock<shared_mutex> lock(ClientMapMutex);
			ClientMap[aircraftID] = new Client(clientSock, aircraftID);
		}
		return 0;
	}

	/**
	 * @brief Server object deconstructor. cleans WSA, closes sockets, and deletes clients
	 */
	~Server() {

		running = false;
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

	/**
	 * @brief Receives from a socket until a specific number of bytes are received.
	 *
	 * @param a client socket
	 * @param b buffer to write to
	 * @param c number of bytes to receive
	 * @return true or false based on whether receive was successful
	 */
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

	/**
	 * @brief Calculates current fuel consumption since last packet
	 *
	 * @param a seconds since last packet
	 * @param b difference in fuel since last packet
	 * @return average fuel consumption float
	 */
	float calculateFuelConsumption(int deltaTime, float deltaFuel) {
		return -deltaFuel / deltaTime;
	}

	/**
	 * @brief Calculates the running average fuel consumption
	 *
	 * @param a total packets received
	 * @param b current running average of fuel consumption
	 * @param c current fuel consumption
	 * @return average fuel consumption float
	 */
	float calculateAverageFuelConsumption(int packets_received, float average_fuel_consumption, float fuel_consumption) {
		if (packets_received == 0) {
			return fuel_consumption;
		}

		return ((average_fuel_consumption * packets_received) + fuel_consumption) / (packets_received + 1);
	}

	/**
	 * @brief accepts the first packet sent by a client to return the client ID
	 *
	 * @param a client socket
	 * @return client ID int
	 */
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

	/**
	 * @brief processes all tasks in TaskQueue. Run as threads.
	 */
	void HandleTasks() {

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


			//check if file is open
			if (!current_client->file.is_open()) {
				current_client->ClientMutex.unlock();
				cout << "ERROR: FILE FAILED TO OPEN: " << current_client->ID << '\n';

				current_client->file.open(LOGFOLDER + to_string(current_client->ID) + ".txt", ios::app);
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

	/**
	 * @brief Cleans up data tied to client when the client disconnects. Blocks all worker threads
	 *
	 * @param a Client ID
	 * @return void
	 */
	void OnClientDisconnect(int clientID) {
		unique_lock<shared_mutex> lock(ClientMapMutex);
		auto it = ClientMap.find(clientID);

		//aircraft has already been removed.
		if (it == ClientMap.end())
			return;
		Client* current_client = it->second;

		//save average fuel consumption
		current_client->ClientMutex.lock();
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

	/**
	 * @brief handles receiving every packet for the server using WSAPoll(). Run as a thread.

	 * @return void
	 */
	void HandleReceive() {
		while (running)
		{
			//makes two new vectors every loop. Can be optimized in the future
			std::vector<WSAPOLLFD> fds;
			std::vector<int> idMap; // maps fds[i] -> client ID

			fds.reserve(ClientMap.size());
			idMap.reserve(ClientMap.size());

			//populate maps
			for (auto& [id, client] : ClientMap)
			{
				WSAPOLLFD fd{};
				fd.fd = client->clientsock;
				fd.events = POLLRDNORM;

				fds.push_back(fd);
				idMap.push_back(id);
			}

			//no clients trying to join
			if (fds.empty())
			{
				Sleep(50);//stop thread from spinning, give time for more sockets to be ready
				continue;
			}

			int ret = WSAPoll(fds.data(), (ULONG)fds.size(), -1);

			if (ret <= 0)
				continue;

			for (size_t i = 0; i < fds.size(); i++)
			{
				short re = fds[i].revents;
				int clientId = idMap[i];

				if (re & (POLLERR | POLLHUP | POLLNVAL))
				{
					OnClientDisconnect(clientId);
					continue;
				}

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
					//push the task
					QueueMutex.lock();
					TaskQueue.push(current_task);
					QueueMutex.unlock();
					TaskCV.notify_one(); //tells 1 thread to wakeup

				}
			}
		}
	}
};


/**
 * @brief main thread
 *
 * @return exit code int. 0 for normal exit, any other number means there was an error
 */
int main() {

	Server server(8080, 12);

	server.Initialize();

	return 0;
}


