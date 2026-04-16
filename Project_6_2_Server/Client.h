#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>

#include "Packets.h"

#define LOGFOLDER "Logs/"

#define TIMESTAMP_SIZE sizeof(int)
#define FUEL_SIZE sizeof(float)
#define AIRCRAFT_ID_SIZE sizeof(int)

class Client {
	SOCKET clientSock;
	char* buffer;

	Client();

private:
	bool recvAll(int totalBytes);
	void handleClient();

};