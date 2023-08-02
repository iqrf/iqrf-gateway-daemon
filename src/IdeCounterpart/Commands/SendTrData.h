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
		SendTrData(const std::basic_string<uint8_t> &data) {
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
	};
}
