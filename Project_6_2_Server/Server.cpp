#include "Server.h"


Server::Server() {
	//create socket
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		std::cerr << "WSAStartup failed\n";
		return;
	}

	serverSock = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSock == INVALID_SOCKET) {
		std::cerr << "Socket creation failed\n";
		WSACleanup();
		return;
	}

	sockaddr_in server{};
	server.sin_family = AF_INET;
	server.sin_port = htons(8080);
	server.sin_addr.s_addr = INADDR_ANY;
}



void Server::OnConnect() {

}
void Server::OnDisconnect() {

}
void Server::TelemetryHandler() {

}
void Server::ConnectionHandler() {

}
void Server::SocketHandler() {

}

DataPacket Server::recvAll(Client client, int NumberOfBytes) {

	int received = 0;
	DataPacket buffer;

	while (received < NumberOfBytes) {
		int r = recv(client.socket, (char*)&buffer + received, NumberOfBytes - received, 0);

		if (r <= 0) {
			return ; //connection closed or error
		}

		received += r;
	}

	return true;
}

float Server::calculateFuelConsumption(Client client) {

}

float Server::calculateAverageFuelConsumption(Client client) {

}