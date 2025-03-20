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

#include <list>
#include <vector>
#include <string>
#include <stdexcept>

#include "CodeBlock.h"

#define BLOCK_SIZE_PREP 64
#define CRC_INIT_VAL_HEX 1
#define CRC_INIT_VAL_IQRF 3

namespace iqrf {
	/// \class PreparedData
	/// \brief Holds data prepared for writing into external EEPROM and loading into
	/// MCU Flash memory on nodes.
	class PreparedData {
	public:
		typedef std::vector<std::basic_string<uint8_t>> Data;

		/**
		 * Constructor
		 * @param data Data
		 * @param len Length of data
		 * @param checksum Checksum value
		 */
		PreparedData(const Data &data, uint16_t len, uint16_t checksum) {
			m_data = data;
			m_length = len;
			m_checksum = checksum;
		}

		/**
		 * Converts Intel HEX code blocks to data suitable for upload to network
		 * @param codeBlocks Intel HEX code blocks
		 * @return PreparedData Data suitable for upload to network
		 */
		static PreparedData fromHex(const std::list<CodeBlock> &codeBlocks) {
			const CodeBlock *block = nullptr;
			for (auto &item : codeBlocks) {
				if (item.getStartAddr() == (0x3A20 * 2)) {
					block = &item;
					break;
				}
			}
			if (block == nullptr) {
				throw std::invalid_argument("Selected hex file does not include Custom DPA handler section or the code does not start with clrwdt() marker.");
			}

			uint16_t len = (block->getLength() + (64 - 1)) & ~(64 - 1);
			uint16_t chksum = checksum(*block, len, CRC_INIT_VAL_HEX);
			Data data = prepareAsMostEffective(*block);
			return PreparedData(data, len, chksum);
		}

		/**
		 * Converts IQRF code blocks to data suitable for upload to network
		 * @param codeBlock IQRF code blocks
		 * @param broadcast Prepare data for network distribution
		 * @return PreparedData Data suitable for upload to network
		 */
		static PreparedData fromIqrf(const CodeBlock &codeBlock, bool broadcast) {
			uint16_t len = codeBlock.getLength();
			uint16_t chksum = checksum(codeBlock, len, CRC_INIT_VAL_IQRF);

			Data data;
			if (broadcast) {
				data = prepareAs16ByteBlocks(codeBlock);
			} else {
				data = prepareAsMostEffective(codeBlock);
			}
			return PreparedData(data, len, chksum);
		}

		/**
		 * Returns prepared data
		 * @return Data Prepared data
		 */
		Data getData() {
			return m_data;
		}

		/**
		 * Returns data length
		 * @return uint16_t Data length
		 */
		uint16_t getLength() {
			return m_length;
		}

		/**
		 * Returns data checksum
		 * @return uint16_t Checksum
		 */
		uint16_t getChecksum() {
			return m_checksum;
		}
	private:
		/**
		 * Computes data checksum
		 * @param block Code block
		 * @param len Data length
		 * @param initVal Initial value
		 * @return uint16_t Checksum
		 */
		static uint16_t checksum(const CodeBlock &block, uint16_t &len, uint16_t initVal) {
			uint16_t sum = initVal;
			uint16_t padding = 0x34FF;
			bool lowerFillingByteOnRank = true;
			size_t idx = 0;

			for (uint16_t addr = block.getStartAddr(), endAddr = block.getStartAddr() + len; addr < endAddr; addr++, idx++) {
				uint8_t byte = 0;
				if (block.getEndAddr() - addr >= 0) {
					byte = block.getCode()[idx] & 0xFF;
				} else {
					if (lowerFillingByteOnRank) {
						byte = padding & 0xFF;
						lowerFillingByteOnRank = false;
					} else {
						byte = (padding >> 8) & 0xFF;
						lowerFillingByteOnRank = true;
					}
				}
				uint16_t lo = sum & 0xFF;
				uint16_t hi = sum >> 8;
				lo += byte;
				if ((lo & 0x100) != 0) {
					lo++;
				}
				hi += lo & 0xFF;
				if ((hi & 0x100) != 0) {
					hi++;
				}
				sum = (lo & 0xFF) | (hi & 0xFF) << 8;
			}
			return sum;
		}

		/**
		 * Prepares data as fixed blocks of 16 bytes for network transfer
		 * @param block Code blocks
		 * @return Data Prepared data
		 */
		static Data prepareAs16ByteBlocks(const CodeBlock &block) {
			Data data;
			std::basic_string<uint8_t> byteBlock;
			uint16_t cnt = 0;

			for (uint16_t addr = block.getStartAddr(), endAddr = block.getEndAddr(); addr <= endAddr;) {
				fillByteBlock(byteBlock, block.getCode(), 16, cnt);
				cnt += 16;
				addr += 16;
			}

			while (((data.size() * 16) % BLOCK_SIZE_PREP) != 0) {
				for (uint8_t i = 0; i < 16; i += 2) {
					byteBlock[i] = 0xFF;
					byteBlock[i + 1] = 0x34;
				}
				data.push_back(byteBlock);
			}
			return data;
		}

		/**
		 * Prepares data as blocks maximizing their capacity
		 * @param block Code blocks
		 * @return Data Prepared data
		 */
		static Data prepareAsMostEffective(const CodeBlock &block) {
			Data data;
			std::basic_string<uint8_t> byteBlock;
			uint16_t cnt = 0;

			for (uint16_t addr = block.getStartAddr(), endAddr = block.getEndAddr(); addr <= endAddr;) {
				fillByteBlock(byteBlock, block.getCode(), 48, cnt);
				data.push_back(byteBlock);
				cnt += 48;

				fillByteBlock(byteBlock, block.getCode(), 16, cnt);
				data.push_back(byteBlock);
				cnt += 16;

				fillByteBlock(byteBlock, block.getCode(), 16, cnt);
				data.push_back(byteBlock);
				cnt += 16;

				fillByteBlock(byteBlock, block.getCode(), 48, cnt);
				data.push_back(byteBlock);
				cnt += 48;

				addr += 128;
			}
			return data;
		}

		/**
		 * Fills block with data
		 * @param byteBlock Block
		 * @param source Source data
		 * @param blockSize Block size
		 * @param offset Data offset
		 */
		static void fillByteBlock(std::basic_string<uint8_t> &byteBlock, const std::basic_string<uint8_t> &source, uint8_t blockSize, uint16_t offset) {
			byteBlock.resize(blockSize);
			for (uint8_t i = 0; i < blockSize; i++)
			{
				if ((offset + i) >= source.size())  {
					if (i & 0x01) {
						byteBlock[i] = 0x34;
					} else {
						byteBlock[i] = 0xff;
					}
				} else {
					byteBlock[i] = source[offset + i];
				}
			}
		}

		// Prepared data
		Data m_data;
		// Data length
		uint16_t m_length;
		// Data checksum
		uint16_t m_checksum;
	};
}
