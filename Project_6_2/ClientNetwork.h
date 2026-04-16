/**
 * @file ClientNetwork.h
 * @brief Declares the ClientNetwork class, which manages the network connection to the server, including connecting, sending packets, and closing the connection.
 * @author Navjot Jandu
 */

#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include "Packets.h"

#pragma comment(lib, "ws2_32.lib")

 /**
 * @class ClientNetwork
 * @brief Manages the network connection to the server, including connecting, sending packets, and closing the connection.
 */
class ClientNetwork
{
public:
	ClientNetwork();
	~ClientNetwork();

	bool Connect(const std::string& serverIP, int port);
	bool SendIDPacket(const IDPacket& packet);
	bool SendDataPacket(const DataPacket packet);
	void Close();

private:
	SOCKET _socket;
};