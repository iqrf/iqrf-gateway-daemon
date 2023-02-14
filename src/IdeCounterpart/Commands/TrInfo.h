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
