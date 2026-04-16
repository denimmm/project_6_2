/**
 * @file TelemetryModule.cpp
 * @brief Implements the TelemetryModule class, which is responsible for reading telemetry data from a randomly selected text file and sending it to the server at regular intervals. The module reads the timestamp and fuel remaining from each line of the file, converts the timestamp to Unix time, and sends this data as a DataPacket to the server every second until the end of the file is reached.
 * @author Jax Drummond
 */

#include "TelemetryModule.h"
#include "Packets.h"
#include <filesystem>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace fs = std::filesystem;

/**
 * @brief Constructs a TelemetryModule object, initializes the telemetry files by scanning the current directory for .txt files, and loads a random file for reading telemetry data.
 */
TelemetryModule::TelemetryModule()
{
	_init();
}

/**
 * @brief Runs the telemetry module by reading telemetry data from the loaded file and sending it to the server using the provided ClientNetwork object. The module reads each line of the file, parses the timestamp and fuel remaining, converts the timestamp to Unix time, creates a DataPacket, and sends it to the server every second until the end of the file is reached.
 * @param client The ClientNetwork object used to send telemetry data packets to the server.
 */
void TelemetryModule::Run(ClientNetwork& client)
{
	_readFile(client); // Opens and reads the randomly chosen file.
}

/**
 * @brief Initializes the telemetry module by scanning the current directory for .txt files, storing their names in a vector, and loading a random file from that vector for reading telemetry data. This function is called in the constructor of the TelemetryModule class to set up the necessary files for telemetry data reading.
 */
void TelemetryModule::_init()
{
	_getFiles();          // Gets the txt files in the running directory.
	_loadRandomFile();    // Loads a random file from the _telemetryFiles vector.
}

/**
 * @brief Scans the current directory for regular files with a .txt extension and stores their filenames in the _telemetryFiles vector. If no .txt files are found, it outputs a message indicating that no telemetry files are available. This function is called during the initialization of the TelemetryModule to prepare a list of potential files to read telemetry data from.
 */
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

/**
 * @brief Loads a random telemetry file from the _telemetryFiles vector. If the vector is empty, it outputs a message indicating that no telemetry files are available. It seeds the random number generator with the current time, selects a random index from the vector, and attempts to open the corresponding file for reading. If the file cannot be opened, it outputs an error message indicating which file failed to open.
 */
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

/**
 * @brief Reads telemetry data from the currently opened file, parses the timestamp and fuel remaining from each line, converts the timestamp to Unix time, creates a DataPacket with this information, and sends it to the server using the provided ClientNetwork object. The function continues to read and send data every second until the end of the file is reached. If the file cannot be opened, it outputs an error message and returns without attempting to read data.
 * @param client The ClientNetwork object used to send telemetry data packets to the server.
 */
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
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	_file.close();
}

/**
 * @brief Converts a timestamp string in the format "dd_mm_yyyy HH:MM:SS" to Unix time (seconds since January 1, 1970). The function uses std::get_time to parse the timestamp into a std::tm structure, and then uses std::mktime to convert it to Unix time. If parsing or conversion fails, it outputs an error message and returns -1.
 * @param timestamp The timestamp string to convert.
 * @return Returns the Unix time on success, -1 on error.
 */
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