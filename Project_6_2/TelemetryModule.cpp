#include "TelemetryModule.h"
#include <dirent.h>
#include "Packets.h"


TelemetryModule::TelemetryModule()
{
	_init();
}

void TelemetryModule::Run()
{
	_readFile(); // Opens and reads the randomly choosen file.
}

void TelemetryModule::_init()
{
	_getFiles(); // Gets the txt file in the running directory.
	_loadRandomFile(); // Loads a random file from the _telemetryFiles vector.
}

void TelemetryModule::_getFiles()
{
	DIR* directory;
	struct dirent* en;

	directory = opendir(".");
	if (directory)
	{
		while ((en = readdir(directory)) != NULL)
		{
			std::string name = std::string(en->d_name);
			if (name.find(".txt") != std::string::npos)
			{
				_telemetryFiles.push_back(name);
			}
		}
		closedir(directory);
	}
	else
	{
		std::cout << "Could not open directory\n";
	}
}

void TelemetryModule::_loadRandomFile()
{
	srand(time(0)); // Use the current time as the seed for random
	int index = rand() % _telemetryFiles.size();
	_file.open(_telemetryFiles[index]);
}

void TelemetryModule::_readFile()
{
	if (!_file.is_open()) {
		std::cout << "Failed to open file :C\n";
	}

	std::string line;
	while (getline(_file, line)) {

		if (_file.eof())
		{
			break;
		}

		// Parse Data
		std::stringstream ss(line);
		std::string timestamp, current_fuel;
		getline(ss, timestamp, ',');
		getline(ss, current_fuel, ',');
		DataPacket* packet = new DataPacket;
		packet->unixTimestamp = _getUnixTime(timestamp);
		packet->fuelRemaining = atof(current_fuel.c_str());
		
		//send packet
		// Needs the network module

	}

	_file.close();

}

int TelemetryModule::_getUnixTime(std::string timestamp)
{
	std::tm time = {};
	std::istringstream ss(timestamp);

	ss >> std::get_time(&time, "%d_%m_%Y %H:%M:%S");

	if (ss.fail()) {
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
