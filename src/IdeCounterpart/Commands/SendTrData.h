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

	/// Send TR data command (asynchronous response to TR write request) class
	class SendTrData : public BaseCommand {
	public:
		/**
		 * Delete default constructor
		 */
		SendTrData() = delete;

		/**
		 * Full constructor
		 * @param data Asynchronous response data
		 */
		SendTrData(const uint8_t mode, const std::basic_string<uint8_t> &data) {
			m_mode = mode;
			m_data = data;
		}

		/**
		 * Destructor
		 */
		virtual ~SendTrData(){};

		/**
		 * Builds a TR data string containing asynchronous DPA message and encodes data into response
		 */
		void buildResponse() override {
			const size_t dataLen = m_data.size();

			m_response.resize(HEADER_SIZE + CRC_SIZE);
			m_response[GW_ADDR] = static_cast<unsigned char>(m_mode);
			m_response[CMD] = static_cast<unsigned char>(SEND_TR_DATA);
			m_response[DLEN_H] = static_cast<unsigned char>((dataLen >> 8) & 0xFF);
			m_response[DLEN_L] = static_cast<unsigned char>(dataLen & 0xFF);

			if (dataLen > 0) {
				m_response.insert(HEADER_SIZE, m_data);
			}

			uint16_t crc = Crc::get().GetCRC_CCITT(reinterpret_cast<unsigned char *>(m_response.data()), HEADER_SIZE + dataLen);
			m_response[HEADER_SIZE + dataLen] = static_cast<unsigned char>((crc >> 8) & 0xFF);
			m_response[HEADER_SIZE + dataLen + 1] = static_cast<unsigned char>(crc & 0xFF);
		}
	private:
		uint8_t m_mode;
	};
}
