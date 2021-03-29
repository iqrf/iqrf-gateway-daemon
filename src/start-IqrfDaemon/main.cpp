#include "VersionInfo.h"
#include <Shaper.h>
#include "ShapeDefines.h"
#include <StaticComponentMap.h>
#include <Trace.h>

TRC_INIT_MNAME("iqrfgd2");
#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
//Shape buffer channel
#define TRC_CHANNEL 0

#ifndef SHAPE_PLATFORM_WINDOWS
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

void pidInit(const std::string &fileName) {
	pid_t pid = 0;
	std::fstream pidFile;

	pidFile.open(fileName, std::fstream::in | std::fstream::out);
	if (pidFile.is_open()) {
		TRC_INFORMATION("PID file found.");
		std::string content;
		std::getline(pidFile, content);
		try {
			pid = std::stoul(content, nullptr, 10);
		} catch (const std::invalid_argument& e) {
			TRC_WARNING("PID file contains invalid content: " << content);
		}
		if (pid > 0 && kill(pid, 0) == 0) {
			TRC_WARNING("PID file contains PID of a running process: " << pid);
		}
		pidFile.close();
		pidFile.open(fileName, std::fstream::out | std::fstream::trunc);
	} else {
		TRC_INFORMATION("PID file does not exist. Creating PID file " << fileName);
		pidFile.open(fileName, std::fstream::out);
	}
	if (pidFile.is_open()) {
		pid = getpid();
		pidFile << std::to_string(pid) << std::endl;
		if (pidFile.bad()) {
			TRC_WARNING("Failed to write to PID file: " << strerror(errno))
		}
		pidFile.close();
	} else {
		TRC_WARNING("Failed to create PID file: " << strerror(errno)); 
	}
}
#else
// dummy for win
void pidInit(const std::string &fileName) {}
#endif

int main(int argc, char** argv) {
	if (argc == 2 && argv[1] == std::string("version")) {
		std::cout << DAEMON_VERSION << std::endl;
		return 0;
	}

	std::ostringstream header;
	header <<
		"============================================================================" << std::endl <<
		PAR(DAEMON_VERSION) << std::endl << std::endl <<
		"Copyright 2015 - 2017 MICRORISC s.r.o." << std::endl <<
		"Copyright 2018 IQRF Tech s.r.o." << std::endl <<
		"============================================================================" << std::endl;

	std::cout << header.str();
	TRC_INFORMATION(header.str());

	pidInit("/var/run/iqrf-gateway-daemon.pid");

	std::cout << "startup ... " << std::endl;
	shapeInit(argc, argv);
	int retval = shapeRun();
	return retval;
}
