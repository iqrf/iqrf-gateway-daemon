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
#include "IIqrfChannelService.h"

/// iqrf namespace
namespace iqrf {
	/// TR write status code enum
	enum TrWriteResult {
		OK = 0x50,
		ERROR_LEN = 0x60,
		ERROR_BUSY = 0x61,
		ERROR_CRC = 0x62,
		ERROR_SERVICE = 0x63,
	};

	/// TR write command class
	class TrWrite : public BaseCommand {
	public:
		/**
		 * Delete default constructor
		 */
		TrWrite() = delete;

		/**
		 * Full constructor
		 * @param message UDP message
		 * @param exclusiveAccess Exclusive access acquired
		 */
		TrWrite(const std::basic_string<uint8_t> &message, bool exclusiveAccess) : BaseCommand(message), m_exclusiveAccess(exclusiveAccess) {
			m_trWrite = true;
		}

		/**
		 * Destructor
		 */
		virtual ~TrWrite(){};

		/**
		 * Builds and encodes TR write response
		 */
		void buildResponse() override {
			m_response = m_header;
			if (m_exclusiveAccess) {
				m_response[SUBCMD] = TrWriteResult::OK;
			} else {
				m_response[SUBCMD] = TrWriteResult::ERROR_SERVICE;
			}

			encodeResponse();
		}
	private:
		/// Exclusive access acquired
		bool m_exclusiveAccess = false;
	};
}
