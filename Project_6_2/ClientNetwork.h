#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include "Packets.h"

#pragma comment(lib, "ws2_32.lib")

class ClientNetwork
{
public:
    ClientNetwork();
    ~ClientNetwork();

    bool Connect(const std::string& serverIP, int port);
    bool SendIDPacket(const IDPacket& packet);
    bool SendDataPacket(const DataPacket& packet);
    void Close();

private:
    SOCKET _socket;
};