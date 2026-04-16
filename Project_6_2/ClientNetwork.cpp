/**
 * @file ClientNetwork.cpp
 * @brief Implements the ClientNetwork class, which manages the network connection to the server, including connecting, sending packets, and closing the connection.
 * @author Navjot Jandu
 */

#include "ClientNetwork.h"
#include <iostream>

 /**
  * @brief Constructs a ClientNetwork object and initializes the socket to an invalid state.
  */
ClientNetwork::ClientNetwork()
{
	_socket = INVALID_SOCKET; //Initiazlise socket to invalid socket
}

/**
  * @brief Destructs the ClientNetwork object and ensures that the network connection is properly closed.
  */
ClientNetwork::~ClientNetwork()
{
	Close();
}

/**
  * @brief Connects to the server using the provided IP address and port number. Initializes WinSock, creates a TCP socket, and attempts to connect to the server. Returns true on successful connection, false on failure.
  * @param serverIP The IP address of the server to connect to.
  * @param port The port number to connect to on the server.
  * @return Returns true on successful connection, false on failure.
  */
bool ClientNetwork::Connect(const std::string& serverIP, int port)
{
	//WinSock initialization
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		std::cout << "WSAStartup failed\n";
		return false;
	}

	//TCP socket creation
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket == INVALID_SOCKET)
	{
		std::cout << "Socket creation failed\n";
		WSACleanup();
		return false;
	}

	//Server address setup

	sockaddr_in server{};
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	if (inet_pton(AF_INET, serverIP.c_str(), &server.sin_addr) <= 0)
	{
		std::cout << "Invalid server IP address\n";
		closesocket(_socket);
		WSACleanup();
		return false;
	}

	//connect to server
	if (connect(_socket, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		closesocket(_socket);
		WSACleanup();
		return false;
	}

	return true;
}

/**
  * @brief Sends an IDPacket to the server. Returns true if the packet was sent successfully, false otherwise.
  * @param packet The IDPacket to send to the server.
  * @return Returns true if the packet was sent successfully, false otherwise.
  */
bool ClientNetwork::SendIDPacket(const IDPacket& packet)
{
	//Aircraft ID packet sending
	int result = send(_socket, reinterpret_cast<const char*>(&packet), sizeof(IDPacket), 0);
	return result == sizeof(IDPacket);
}

/**
  * @brief Sends a DataPacket to the server. Returns true if the packet was sent successfully, false otherwise.
  * @param packet The DataPacket to send to the server.
  * @return Returns true if the packet was sent successfully, false otherwise.
  */
bool ClientNetwork::SendDataPacket(const DataPacket packet)
{
	//Telemetry data packet sending
	int result = send(_socket, reinterpret_cast<const char*>(&packet), sizeof(DataPacket), 0);
	return result == sizeof(DataPacket);
}

/**
  * @brief Closes the network connection to the server. If the socket is valid, it closes the socket and cleans up WinSock resources.
  */
void ClientNetwork::Close()
{
	if (_socket != INVALID_SOCKET)
	{
		closesocket(_socket);
		_socket = INVALID_SOCKET;
		WSACleanup();
	}
}