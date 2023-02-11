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
#pragma once

#include "BaseCommand.h"
#include "IIqrfDpaService.h"
#include "VersionInfo.h"

/// iqrf namespace
namespace iqrf {
	/// Gateway identification parameters
	struct GwIdentParams {
		uint8_t mode;
		std::string name;
		std::string ipStack;
		std::string netBios;
		std::string publicIp;
		std::string IP;
		std::string MAC;
	};

	/// Gateway identification command class
	class GatewayIdentification : public BaseCommand {
	public:
		/**
		 * Delete default constructor
		 */
		GatewayIdentification() = delete;

		/**
		 * Full constructor
		 * @param message UDP message
		 * @param params Gateway indentification parameters
		 * @param dpaService DPA service interface
		 */
		GatewayIdentification(const std::basic_string<uint8_t> &message, GwIdentParams &params, IIqrfDpaService *dpaService) : BaseCommand(message) {
			m_dpaService = dpaService;
			m_params = params;
		}

		/**
		 * Destructor
		 */
		virtual ~GatewayIdentification(){};

		/**
		 * Builds gateway identification data string and encodes data into response
		 */
		void buildResponse() override {
			auto coordinatorParams = m_dpaService->getCoordinatorParameters();

			std::basic_ostringstream<char> ss;
			ss << crlf
				<< m_params.name << crlf
				<< DAEMON_VERSION << crlf
				<< m_params.MAC << crlf
				<< m_params.ipStack << crlf
				<< m_params.IP << crlf
				<< m_params.netBios << crlf
				<< coordinatorParams.osVersion << "(" << coordinatorParams.osBuild << ")" << crlf
				<< m_params.publicIp << crlf;
			std::string data = ss.str();
			m_data = std::basic_string<unsigned char>((unsigned char *)data.data(), data.size());

			encodeResponse();
		}
	private:
		/// EOL for identification data string
		const char* crlf = "\x0D\x0A";
		/// Gateway identification parameters
		GwIdentParams m_params;
		/// DPA service interface
		IIqrfDpaService *m_dpaService = nullptr;
	};
}
