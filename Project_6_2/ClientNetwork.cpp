#include "ClientNetwork.h"
#include <iostream>

ClientNetwork::ClientNetwork()
{
	_socket = INVALID_SOCKET; //Initiazlise socket to invalid socket
}

ClientNetwork::~ClientNetwork()
{
	Close();
}

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

bool ClientNetwork::SendIDPacket(const IDPacket& packet)
{
	//Aircraft ID packet sending
	int result = send(_socket, reinterpret_cast<const char*>(&packet), sizeof(IDPacket), 0);
	return result == sizeof(IDPacket);
}

bool ClientNetwork::SendDataPacket(const DataPacket& packet)
{
	//Telemetry data packet sending
	int result = send(_socket, reinterpret_cast<const char*>(&packet), sizeof(DataPacket), 0);
	return result == sizeof(DataPacket);
}

void ClientNetwork::Close()
{
	if (_socket != INVALID_SOCKET)
	{
		closesocket(_socket);
		_socket = INVALID_SOCKET;
		WSACleanup();
	}
}