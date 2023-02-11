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

#include <ctime>
#include <string>

#include "BaseCommand.h"

/// iqrf namespace
namespace iqrf {

	/// Gateway status items enum
	enum StatusData {
		TR_STATUS,
		UNUSED2,
		SUPPLY_EXT,
		SECONDS,
		MINUTES,
		HOURS,
		WEEK_DAY,
		MONTH_DAY,
		MONTH,
		YEAR,
		UNUSED11,
		UNUSED12,
	};

	/// Gateway status command class
	class GatewayStatus : public BaseCommand {
	public:
		/**
		 * Delete default constructor
		 */
		GatewayStatus() = delete;

		/**
		 * Full constructor
		 * @param message UDP message
		 * @param exclusiveAccess Exclusive access acquired
		 */
		GatewayStatus(const std::basic_string<uint8_t> &message, bool exclusiveAccess) : BaseCommand(message) {
			m_exclusiveAccess = exclusiveAccess;
		}

		/**
		 * Destructor
		 */
		virtual ~GatewayStatus(){};

		/**
		 * Builds gateway status data string and encodes data into response
		 */
		void buildResponse() override {
			std::time_t now = std::time(0);
			std::tm *time = std::localtime(&now);

			m_data.resize(len);
			m_data[TR_STATUS] = m_exclusiveAccess ? 0x80 : 0xFF;
			m_data[SUPPLY_EXT] = 0x01;
			m_data[SECONDS] = static_cast<unsigned char>(time->tm_sec);
			m_data[MINUTES] = static_cast<unsigned char>(time->tm_min);
			m_data[HOURS] = static_cast<unsigned char>(time->tm_hour);
			m_data[WEEK_DAY] = static_cast<unsigned char>(time->tm_wday);
			m_data[MONTH_DAY] = static_cast<unsigned char>(time->tm_mday);
			m_data[MONTH] = static_cast<unsigned char>(time->tm_mon);
			m_data[YEAR] = static_cast<unsigned char>(time->tm_year % 100);

			encodeResponse();
		}

	private:
		/// Exclusive access acquired
		bool m_exclusiveAccess = false;
		/// Data length
		static const uint8_t len = 12;
	};
}
