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

/// iqrf namespace
namespace iqrf {

	/// TR reset command class
	class TrReset : public BaseCommand {
	public:
		/**
		 * Delete default constructor
		 */
		TrReset() = delete;

		/**
		 * Full constructor
		 * @param message DPA message
		 * @param exclusiveAccess Exclusive access acquired
		 */
		TrReset(const std::basic_string<uint8_t> &message, bool exclusiveAccess) : BaseCommand(message), m_exclusiveAccess(exclusiveAccess) {
			m_trWrite = true;
		}

		/**
		 * Destructor
		 */
		virtual ~TrReset(){};

		/**
		 * Returns TR reset DPA request
		 * @return TR reset DPA request
		 */
		static std::basic_string<unsigned char> getDpaRequest() {
			return std::basic_string<unsigned char>{0x00, 0x00, 0x02, 0x01, 0xff, 0xff};
		}

		/**
		 * Builds and encodes TR reset response
		 */
		void buildResponse() override {
			m_response = m_header;
			if (m_exclusiveAccess) {
				m_response[SUBCMD] = PACKET_OK;
			} else {
				m_response[SUBCMD] = PACKET_ERROR;
			}

			encodeResponse();
		}

	private:
		/// Exclusive access acquired
		bool m_exclusiveAccess = false;
	};
}
