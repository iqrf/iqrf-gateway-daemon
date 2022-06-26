/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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

		PreparedData(const Data &data, uint16_t len, uint16_t checksum) {
			m_data = data;
			m_length = len;
			m_checksum = checksum;
		}

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

		Data getData() { return m_data; }
		uint16_t getChecksum() { return m_checksum; }
		uint16_t getLength() { return m_length; }
	private:
		Data m_data;
		uint16_t m_length;
		uint16_t m_checksum;

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
	};
}
