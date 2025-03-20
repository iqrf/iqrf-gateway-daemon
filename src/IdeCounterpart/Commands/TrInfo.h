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

/// iqrf namespace
namespace iqrf {

	/// TR information command class
	class TrInfo : public BaseCommand {
	public:
		/**
		 * Delete default constructor
		 */
		TrInfo() = delete;

		/**
		 * Full constructor
		 * @param message UDP message
		 * @param dpaService DPA service interface
		 */
		TrInfo(const std::basic_string<uint8_t> &message, IIqrfDpaService *dpaService) : BaseCommand(message), m_dpaService(dpaService) {}

		/**
		 * Destructor
		 */
		virtual ~TrInfo(){};

		/**
		 * Builds TR information data string and encodes data into response
		 */
		void buildResponse() override {
			auto coordinatorParams = m_dpaService->getCoordinatorParameters();

			m_data.resize(8);
			uint32_t mid = coordinatorParams.mid;
			m_data[0] = static_cast<uint8_t>((mid >> 24) & 0xFF);
			m_data[1] = static_cast<uint8_t>((mid >> 16) & 0xFF);
			m_data[2] = static_cast<uint8_t>((mid >> 8) & 0xFF);
			m_data[3] = static_cast<uint8_t>(mid & 0xFF);
			m_data[4] = coordinatorParams.osVersionByte;
			m_data[5] = coordinatorParams.trMcuType;
			m_data[6] = static_cast<uint8_t>(coordinatorParams.osBuildWord & 0xFF);
			m_data[7] = static_cast<uint8_t>((coordinatorParams.osBuildWord >> 8) & 0xFF);

			encodeResponse();
		}
	private:
		/// DPA service interface
		IIqrfDpaService *m_dpaService = nullptr;
	};
}
