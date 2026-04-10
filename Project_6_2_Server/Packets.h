#pragma once

typedef struct IDPacket
{
	int aircraftID;
};

typedef struct DataPacket
{
	int unixTimestamp;
	float fuelRemaining;
};

