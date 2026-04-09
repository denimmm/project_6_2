#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include"ClientNetwork.h"

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