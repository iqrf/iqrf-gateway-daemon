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

#include <string>
#include "crc.h"

/// iqrf namespace
namespace iqrf {
	/// Packet header item enum
	enum PacketHeader {
		/// Identification address of device: 0x22 for IQRF device, 0x20 for 3rd party or user devices
		GW_ADDR,
		/// Packet command, response: CMD = CMD | 0x80
		CMD,
		/// Auxiliary data for a command
		SUBCMD,
		/// Reserved
		RES0,
		/// Reserved
		RES1,
		/// Packet ID, upper byte: 0x00 - 0xFF
		PACID_H,
		/// Packet ID, lower byte: 0x00 - 0xFF
		PACID_L,
		/// Packet data length, upper byte: 0x00, 0x01
		DLEN_H,
		/// Packet data length, lower byte: 0x00 - 0xFF
		DLEN_L,
	};

	/// UDP commands enum
	enum UdpCommands {
		/// Gateway identification command
		GW_IDENTIFICATION = 0x01,
		/// Gateway status command
		GW_STATUS,
		/// Write data to TR module command
		TR_WRITE,
		/// Send data from TR module to IDE command
		SEND_TR_DATA,
		/// TR module information command
		TR_INFO = 0x11,
		/// TR reset command
		TR_RESET = 0x13,
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
		/// UDP packet ok code
		const static uint8_t PACKET_OK = 0x50;
		/// UDP packet error code
		const static uint8_t PACKET_ERROR = 0x60;
		/// UDP packet response command code
		const static uint8_t PACKET_RESPONSE_CODE = 0x80;

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
		bool isTrWriteRequired() {
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
			m_response[CMD] |= PACKET_RESPONSE_CODE;
			if (write) {
				m_response[SUBCMD] = subcmd;
			}
			m_response[DLEN_H] = static_cast<unsigned char>((dataLen >> 8) & 0xFF);
			m_response[DLEN_L] = static_cast<unsigned char>(dataLen & 0xFF);

			if (dataLen > 0) {
				m_response.insert(HEADER_SIZE, m_data);
			}

			uint16_t crc = Crc::get().GetCRC_CCITT(reinterpret_cast<unsigned char *>(m_response.data()), HEADER_SIZE + dataLen);
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
