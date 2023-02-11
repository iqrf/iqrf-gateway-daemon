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

#include <string>
#include "crc.h"

/// iqrf namespace
namespace iqrf {
	/// Packet header item enum
	enum PacketHeader {
		GW_ADDR,
		CMD,
		SUBCMD,
		RES0,
		RES1,
		PACID_H,
		PACID_L,
		DLEN_H,
		DLEN_L
	};

	/// UDP commands enum
	enum UdpCommands {
		GW_IDENTIFICATION = 0x01,
		GW_STATUS,
		TR_WRITE,
		SPI_DATA,
		GW_STATUS_ASYNC,
		RTCC_WRITE = 0x08,
		CHANGE_AUTENTIZATION,
		TR_INFO = 0x11,
		GW_RESET,
		TR_RESET
	};

	/// UDP command base class
	class BaseCommand {
	public:
		/// UDP packet header size
		const static uint8_t HEADER_SIZE = 9;
		/// UDP packet maximum data size
		const static uint16_t DATA_MAX_SIZE = 497;
		/// UDP packet CRC size
		const static uint8_t CRC_SIZE = 2;
		/// UDP packet error code
		const static uint8_t PACKET_ERROR = 0x60;

		/**
		 * Default constructor
		 */
		BaseCommand(){};

		/**
		 * Message constructor, stores header for later use
		 * @param message UDP message
		 */
		BaseCommand(const std::basic_string<uint8_t> message) {
			m_header = message.substr(0, HEADER_SIZE);
		}

		/**
		 * Destructor
		 */
		virtual ~BaseCommand(){};

		/**
		 * Returns UDP response message
		 * @return UDP response message
		 */
		const std::basic_string<unsigned char> getResponse() {
			return m_response;
		}

		/**
		 * Indicates whether UDP message requires write to TR
		 * @return true if writing to TR is required, false otherwise
		 */
		bool shouldTrWrite() {
			return m_trWrite;
		}

		/**
		 * Handles command and builds response for IDE, implemented in individual command classes
		 */
		virtual void buildResponse() = 0;
	protected:
		/**
		 * Encodes response data into a packet, updates header parameters and calculates new CRC
		 */
		void encodeResponse() {
			const size_t dataLen = m_data.size();

			bool write = m_response[CMD] == TR_WRITE;
			uint8_t subcmd = write ? m_response[SUBCMD] : 0;

			m_response = m_header;

			m_response.resize(HEADER_SIZE + CRC_SIZE);
			m_response[CMD] = m_response[CMD] | 0x80;
			if (write) {
				m_response[SUBCMD] = subcmd;
			}
			m_response[DLEN_H] = static_cast<unsigned char>((dataLen >> 8) & 0xFF);
			m_response[DLEN_L] = static_cast<unsigned char>(dataLen & 0xFF);

			if (dataLen > 0) {
				m_response.insert(HEADER_SIZE, m_data);
			}

			uint16_t crc = Crc::get().GetCRC_CCITT((unsigned char *)m_response.data(), HEADER_SIZE + dataLen);
			m_response[HEADER_SIZE + dataLen] = static_cast<unsigned char>((crc >> 8) & 0xFF);
			m_response[HEADER_SIZE + dataLen + 1] = static_cast<unsigned char>(crc & 0xFF);
		}

		/// Packet header
		std::basic_string<unsigned char> m_header;
		/// Message data
		std::basic_string<unsigned char> m_data;
		/// UDP response
		std::basic_string<unsigned char> m_response;
		/// Indicates whether command requires writing to TR
		bool m_trWrite = false;
	};
}