/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "VersionInfo.h"
#include <Shaper.h>
#include "ShapeDefines.h"
#include <StaticComponentMap.h>
#include <Trace.h>
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>

TRC_INIT_MNAME("iqrfgd2")
#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
//Shape buffer channel
#define TRC_CHANNEL 0

#ifndef SHAPE_PLATFORM_WINDOWS
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

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

namespace bpo = boost::program_options;

int main(int argc, char** argv) {
	try {
		bpo::options_description description("Options");
		description.add_options()
			("help,h", "Show help and usage")
			("version,v", "Show version")
			("configuration,c",
			 bpo::value<std::string>()->default_value("/etc/iqrf-gateway-daemon.json"),
			 "Path to configuration file, required to start")
			("pidfile,p",
			 bpo::value<std::string>()->default_value("/var/run/iqrf-gateway-daemon.pid"),
			 "Path to PID file");
		bpo::variables_map vm;
		bpo::store(bpo::parse_command_line(argc, argv, description), vm);
		bpo::notify(vm);
		if (argc == 1 || vm.count("help")) {
			std::cout << "Usage: iqrfgd2" << " [options]" << std::endl << description << std::endl;
		} else if (vm.count("version")) {
			std::cout << DAEMON_VERSION << std::endl;
		} else {
			std::vector<char *> args;
			args.push_back(argv[0]);
			if (argc == 2) {
				std::ifstream fileStream(argv[1]);
        if (!fileStream.good()) {
					std::cerr << "Configuration file " << argv[1] << " does not exist." << std::endl;
					return EXIT_FAILURE;
				}
				args.push_back(argv[1]);
			} else {
				args.push_back(const_cast<char *>(vm["configuration"].as<std::string>().data()));
			}

			std::ostringstream header;
			header <<
				"============================================================================" << std::endl <<
				PAR(DAEMON_VERSION) << std::endl << std::endl <<
				"Copyright 2015 - 2023 IQRF Tech s.r.o." << std::endl <<
				"Copyright 2019 - 2023 MICRORISC s.r.o." << std::endl <<
				"============================================================================" << std::endl;

			std::cout << header.str();
			TRC_INFORMATION(header.str());

			pidInit(vm["pidfile"].as<std::string>());

			std::cout << "startup ... " << std::endl;
			shapeInit(2, args.data());
			int retval = shapeRun();
			return retval;
		}
	} catch (const bpo::error &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
