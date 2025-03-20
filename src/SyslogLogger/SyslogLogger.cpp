/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define ITraceService_EXPORTS
#include "SyslogLogger.h"
#include "TraceLevel.h"
#include "ShapePropertiesMacros.h"
#include "ShapeDefines.h"
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <syslog.h>

#include "iqrf__SyslogLogger.hxx"

TRC_INIT_MODULE(iqrf::SyslogLogger)

namespace iqrf {

	class SyslogLogger::Imp {
	private:
		std::mutex m_mtx;
		std::map<int, int> m_traceLevelMap;
		std::map<TraceLevel, unsigned int> m_traceSyslogLevelMap = {
			{TraceLevel::Error, LOG_ERR},
			{TraceLevel::Warning, LOG_WARNING},
			{TraceLevel::Information, LOG_INFO},
			{TraceLevel::Debug, LOG_DEBUG}
		};
		std::string message;
	public:
		Imp() {
		}

		~Imp() {
			closelog();
		}

		bool isValid(int level, int chan) const {
			auto it = m_traceLevelMap.find(chan);
			return (it != m_traceLevelMap.end() && it->second >= level);
		}

		void writeMsg(int level, int channel, std::string moduleName, std::string sourceFile, int sourceLine, const char* funcName, const std::string & msg) {
			std::lock_guard<std::mutex> guard(m_mtx);
			if (isValid(level, channel)) {
				std::stringstream ss;
				std::size_t idx = sourceFile.find_last_of("/\\");
				if (idx != std::string::npos) {
					sourceFile.erase(0, idx + 1);
				}
				ss << '[' << TraceLevelStringConvertor::int2str(level) << "] ";
				if (!moduleName.empty()) {
					ss << moduleName << '.';
				}
				ss << sourceFile << ':' << funcName << "():" << sourceLine << ' ' << msg;
				syslog(m_traceSyslogLevelMap[static_cast<TraceLevel>(level)], "%s", ss.str().c_str());
			}
		}

		void modify(const Properties *props) {
			std::lock_guard<std::mutex> guard(m_mtx);
			if (props) {
				try {
					std::vector<std::unique_ptr<Properties>> vlvVect;
					GET_MEMBER_AS_VECTOR(*props, Properties, "VerbosityLevels", "", vlvVect);
					for (auto & vlv : vlvVect) {
						int channel;
						GET_MEMBER_AS(*vlv, Int, "channel", "", channel);
						std::string lvlStr;
						GET_MEMBER_AS(*vlv, String, "level", "", lvlStr);
						TraceLevel lvl = TraceLevelStringConvertor::str2enum(lvlStr);
						m_traceLevelMap[channel] = (int)lvl;
					}
					openlog(nullptr, LOG_PID, LOG_LOCAL0);
				} catch (std::exception& e) {
					std::cout << e.what() << std::endl;
				}
			}
		}
	};

	SyslogLogger::SyslogLogger() {
		imp = shape_new Imp();
	}

	SyslogLogger::~SyslogLogger() {
		delete imp;
	}

	bool SyslogLogger::isValid(int level, int chan) const {
		return imp->isValid(level, chan);
	}

	void SyslogLogger::writeMsg(int level, int channel, const char* moduleName, const char* sourceFile, int sourceLine, const char* funcName, const std::string & msg) {
		imp->writeMsg(level, channel, std::string(moduleName), std::string(sourceFile), sourceLine, funcName, msg);
	}

	void SyslogLogger::activate(const Properties *props) {
		imp->modify(props);
	}

	void SyslogLogger::deactivate() {
	}

	void SyslogLogger::modify(const Properties *props) {
		imp->modify(props);
	}

}
