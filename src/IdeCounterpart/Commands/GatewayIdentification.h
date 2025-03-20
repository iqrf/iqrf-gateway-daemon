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
#pragma once

#include "BaseCommand.h"
#include "IIqrfDpaService.h"
#include "VersionInfo.h"

/// iqrf namespace
namespace iqrf {
	/// Gateway identification parameters
	struct GwIdentParams {
		/// Gateway identification mode (0x20, 0x22)
		uint8_t mode;
		/// Gateway type / name (IQD-GW-02)
		std::string name;
		/// TCP/IP stack version (5.42)
		std::string ipStack;
		/// Net BIOS name
		std::string netBios;
		/// Gateway public IP address
		std::string publicIp;
		/// Gateway IP address
		std::string IP;
		/// Gateway MAC address
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
		GatewayIdentification(const std::basic_string<uint8_t> &message, GwIdentParams &params, IIqrfDpaService *dpaService) : BaseCommand(message), m_params(params), m_dpaService(dpaService) {}

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
			m_data = std::basic_string<unsigned char>(reinterpret_cast<unsigned char *>(data.data()), data.size());

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
