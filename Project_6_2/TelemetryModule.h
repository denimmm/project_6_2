/**
 * @file Packets.h
 * @brief Defines the network packet structures for Project_6_2_Client.
 * @author Jax Drummond
 */

#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include"ClientNetwork.h"

/**
* @class TelemetryModule
* @brief The TelemetryModule class is responsible for reading telemetry data from a randomly selected text file and sending it to the server at regular intervals. It scans the current directory for .txt files, loads a random file, reads the timestamp and fuel remaining from each line, converts the timestamp to Unix time, and sends this data as a DataPacket to the server every second until the end of the file is reached.
*/
class TelemetryModule
{
	public:
		TelemetryModule();
		void Run(ClientNetwork& client);
		
	private:
		std::vector<std::string> _telemetryFiles;
		std::fstream _file;
		void _init();
		void _getFiles();
		void _loadRandomFile();
		void _readFile(ClientNetwork& client);
		int _getUnixTime(std::string timestamp);
};