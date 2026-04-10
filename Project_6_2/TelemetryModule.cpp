#include "TelemetryModule.h"
#include "Packets.h"

#include <filesystem>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace fs = std::filesystem;

TelemetryModule::TelemetryModule()
{
	_init();
}

void TelemetryModule::Run(ClientNetwork& client)
{
	_readFile(client); // Opens and reads the randomly chosen file.
}

void TelemetryModule::_init()
{
	_getFiles();          // Gets the txt files in the running directory.
	_loadRandomFile();    // Loads a random file from the _telemetryFiles vector.
}

void TelemetryModule::_getFiles()
{
	for (const auto& entry : fs::directory_iterator("."))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".txt")
		{
			_telemetryFiles.push_back(entry.path().filename().string());
		}
	}

	if (_telemetryFiles.empty())
	{
		std::cout << "No .txt files found in directory\n";
	}
}

void TelemetryModule::_loadRandomFile()
{
	if (_telemetryFiles.empty())
	{
		std::cout << "No telemetry files available\n";
		return;
	}

	srand(time(0)); // Seed random
	int index = rand() % _telemetryFiles.size();

	_file.open(_telemetryFiles[index]);

	if (!_file.is_open())
	{
		std::cout << "Failed to open file: " << _telemetryFiles[index] << "\n";
	}
}

void TelemetryModule::_readFile(ClientNetwork& client)
{
	if (!_file.is_open())
	{
		std::cout << "Failed to open file :C\n";
		return;
	}

	std::string line;
	while (getline(_file, line))
	{
		if (_file.eof())
		{
			break;
		}

		// Parse Data
		std::stringstream ss(line);
		std::string timestamp, current_fuel;

		getline(ss, timestamp, ',');
		getline(ss, current_fuel, ',');

		// Create telemetry packet 
		DataPacket packet;
		packet.unixTimestamp = _getUnixTime(timestamp);
		packet.fuelRemaining = atof(current_fuel.c_str());

		// Send telemetry packet to server
		if (!client.SendDataPacket(packet))
		{
			std::cout << "Failed to send data packet\n";
			break;
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	_file.close();
}

int TelemetryModule::_getUnixTime(std::string timestamp)
{
	std::tm time = {};
	std::istringstream ss(timestamp);

	ss >> std::get_time(&time, "%d_%m_%Y %H:%M:%S");

	if (ss.fail())
	{
		std::cerr << "Error: Failed to parse the time string." << std::endl;
		return -1;
	}

	int unixTime = std::mktime(&time);

	if (unixTime == -1)
	{
		std::cerr << "Error: Failed to convert to Unix time." << std::endl;
		return -1;
	}

	return unixTime;
}