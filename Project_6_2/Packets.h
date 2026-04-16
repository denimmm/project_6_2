/**
 * @file Packets.h
 * @brief Defines the network packet structures for Project_6_2_Server.
 * @author Jax Drummond
 */
#pragma once

 /**
  * @struct IDPacket
  * @brief Defines the structure of the ID packet sent from the client to the server, containing the aircraft ID as an integer.
  */
typedef struct IDPacket
{
	int aircraftID; ///< The unique identifier for the aircraft, sent from the client to the server in the ID packet.
};

/**
 * @struct DataPacket
 * @brief Defines the structure of the data packet sent from the client to the server, containing the Unix timestamp and fuel remaining as a float. The Unix timestamp represents the time at which the telemetry data was recorded, and the fuel remaining indicates how much fuel is left in the aircraft at that time. This packet is sent from the client to the server every second until the end of the telemetry file is reached.
 */
typedef struct DataPacket
{
	int unixTimestamp; ///< The Unix timestamp representing the time at which the telemetry data was recorded.
	float fuelRemaining; ///< The amount of fuel remaining in the aircraft at the time of the telemetry data.
};

