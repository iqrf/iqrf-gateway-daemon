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

#include <cstring>
#include <fstream>
#include <list>
#include <memory>

#include "CodeBlock.h"
#include "IOtaUploadService.h"
#include "StringUtils.h"

#include "iqrf_header_parser.h"

namespace ihp = iqrf_header_parser;

/// iqrf namespace
namespace iqrf {
	/// Intel HEX parser
	class IntelHexParser {
	public:
		/**
		 * Constructor
		 * @param filename Path to HEX DPA handler file
		 */
		IntelHexParser(const std::string &filename) {
			std::ifstream file(filename);
			if (!file.is_open()) {
				throw std::logic_error("Unable to open file " + filename + ": " + std::strerror(errno));
			}

			std::string record;
			while (std::getline(file, record)) {
				StringUtils::trim(record);
				if (record.empty()) {
					continue;
				}
				ihp::hex::validateRecord(record);
				if (std::regex_match(record, std::regex(ihp::hex::COMPATIBILITY_RECORD_HEADER_PATTERN, std::regex_constants::icase))) {
					m_identificationRecord = true;
					ihp::hex::parseCompatibilityHeader(record, m_os, m_mcu, m_tr);
				}
				m_contents.push_back(record);
			}
			file.close();
		}

		/**
		 * Parses HEX DPA handler and returns flash data
		 * @return std::list<CodeBlock> Flash data blocks
		 */
		std::list<CodeBlock> getFlashData() {
			return parse(IOtaUploadService::MemoryType::flash);
		}

		/**
		 * Parses HEX DPA handler and returns EEPROM data
		 * @return std::list<CodeBlock> EEPROM data blocks
		 */
		std::list<CodeBlock> getEepromData() {
			return parse(IOtaUploadService::MemoryType::eeprom);
		}

		/**
		 * Parses HEX DPA handler and returns EEEPROM data
		 * @return std::list<CodeBlock> EEEPROM data blocks
		 */
		std::list<CodeBlock> getEeepromData() {
			return parse(IOtaUploadService::MemoryType::eeeprom);
		}

		/**
		 * Returns dpa handler module compatibility information
		 * @return ihp::device::ModuleInfo Compatibility information
		 */
		ihp::device::ModuleInfo getHeaderModuleInfo() {
			ihp::device::ModuleInfo moduleInfo = ihp::device::ModuleInfo();
			moduleInfo.osMajor = m_os / 10 % 10;
			moduleInfo.osMinor = m_os % 10;
			moduleInfo.mcuType = m_mcu;
			moduleInfo.trSeries = m_tr;
			return moduleInfo;
		}

		/**
		 * Doex Hex DPA handler contain compatibility header?
		 * @return true Compatibility header exists
		 * @return false Compatibility header does not exist
		 */
		bool hasCompatibilityHeader() {
			return m_identificationRecord;
		}
	private:
		/**
		 * Parses HEX DPA handler and returns requested data type in code blocks
		 * @param type Data type
		 * @return std::list<CodeBlock> Code blocks
		 */
		std::list<CodeBlock> parse(IOtaUploadService::MemoryType type) {
			m_codeBlocks.clear();
			uint32_t offset = 0;

			for (auto &record : m_contents) {
				uint8_t byteCount = ihp::utils::hexStringToByte(record, 1);
				uint8_t addrHi = ihp::utils::hexStringToByte(record, 3);
				uint8_t addrLo = ihp::utils::hexStringToByte(record, 5);
				uint8_t recordType = ihp::utils::hexStringToByte(record, 7);
				uint32_t startAddr = 0;

				std::unique_ptr<CodeBlock> block;

				if (recordType == 0) {
					startAddr = (offset + ((addrHi & 0xFF) << 8) + (addrLo & 0xFF));
					if (startAddr == ihp::hex::IDENTIFICATION_HEADER_ADDRESS) {
						continue;
					}

					std::basic_string<uint8_t> data;
					for (uint8_t i = 0; i < byteCount; i++) {
						uint8_t byte = ihp::utils::hexStringToByte(record, i * 2 + 9);
						data.push_back(byte);
					}

					uint32_t endAddr = startAddr + byteCount - 1;
					uint32_t realStartAddr = startAddr >> 1;
					uint32_t realEndAddr = endAddr >> 1;

					if (type == IOtaUploadService::MemoryType::flash) {
						const uint16_t mainAddrMax = (m_mcu == ihp::device::MCU::PIC16LF18877) ? IOtaUploadService::MemoryRanges::IQRF_MAIN_MEM_MAX_ADR_G : IOtaUploadService::MemoryRanges::IQRF_MAIN_MEM_MAX_ADR_D;
						if (realEndAddr <= mainAddrMax) {
							block = std::unique_ptr<CodeBlock>(new CodeBlock(data, (uint16_t)startAddr, (uint16_t)endAddr));
						}
					} else {
						std::basic_string<uint8_t> eeData;
						for (uint8_t i = 0; i < data.length(); i += 2) {
							eeData.push_back(data[i]);
						}
						if (type == IOtaUploadService::MemoryType::eeprom) {
							if ((realStartAddr >= IOtaUploadService::MemoryRanges::INTERNAL_EEPROM_BOTTOM) &&
								(realEndAddr <= IOtaUploadService::MemoryRanges::INTERNAL_EEPROM_TOP)) {
								realStartAddr -= IOtaUploadService::MemoryRanges::INTERNAL_EEPROM_BOTTOM;
								realEndAddr -= IOtaUploadService::MemoryRanges::INTERNAL_EEPROM_BOTTOM;
								block = std::unique_ptr<CodeBlock>(new CodeBlock(eeData, (uint16_t)realStartAddr, (uint16_t)realEndAddr));
							}
						} else {
							if ((realStartAddr >= IOtaUploadService::MemoryRanges::SERIAL_EEPROM_BOTTOM) &&
								(realEndAddr <= IOtaUploadService::MemoryRanges::SERIAL_EEPROM_TOP)) {
								realStartAddr -= IOtaUploadService::MemoryRanges::SERIAL_EEPROM_BOTTOM;
								realEndAddr -= IOtaUploadService::MemoryRanges::SERIAL_EEPROM_BOTTOM;
								block = std::unique_ptr<CodeBlock>(new CodeBlock(eeData, (uint16_t)realStartAddr, (uint16_t)realEndAddr));
							}
						}
					}
				} else if (recordType == 2 || recordType == 4) {
					offset = ((ihp::utils::hexStringToByte(record, 9) & 0xFF) << 8) + (ihp::utils::hexStringToByte(record, 11) & 0xFF);
					offset *= (recordType == 2) ? 16 : 65536;
				}
				if (block) {
					addCodeBlock(*block);
				}
			}
			return m_codeBlocks;
		}

		/**
		 * Adds code block to list of blocks in the correct order
		 * @param block Block to add
		 */
		void addCodeBlock(CodeBlock &block) {
			std::list<CodeBlock>::iterator itr = m_codeBlocks.begin();
			while (itr != m_codeBlocks.end()) {
				if (adjacentBlocks(*itr, block)) {
					CodeBlock merged = mergeBlocks(block, *itr);
					m_codeBlocks.erase(itr++);
					m_codeBlocks.push_back(merged);
					return;
				} else {
					itr++;
				}
			}
			m_codeBlocks.push_back(block);
		}

		/**
		 * Checks if two blocks of code are adjacent
		 * @param b1 Block
		 * @param b2 Block
		 * @return true Blocks are adjacent
		 * @return false Blocks are not adjacent
		 */
		bool adjacentBlocks(const CodeBlock& b1, const CodeBlock& b2) {
			return(((b1.getEndAddr() + 1) == b2.getStartAddr()) || ((b2.getEndAddr() + 1) == b1.getStartAddr()));
		}

		/**
		 * Merges adjacent blocks to a single block
		 * @param b1 Block to merge
		 * @param b2 Block to merge
		 * @return CodeBlock New block
		 */
		CodeBlock mergeBlocks(const CodeBlock& b1, const CodeBlock& b2) {
			if ((b1.getEndAddr() + 1) == b2.getStartAddr()) {
				return CodeBlock(b1.getCode() + b2.getCode(), b1.getStartAddr(), b2.getEndAddr());
			} else {
				return CodeBlock(b2.getCode() + b1.getCode(), b2.getStartAddr(), b1.getEndAddr());
			}
		}

		/// List of code records
		std::list<std::string> m_contents;
		/// List of code blocks
		std::list<CodeBlock> m_codeBlocks;
		/// Indicates presence of identification record in the hex file
		bool m_identificationRecord;
		/// Header MCU
		uint8_t m_mcu;
		/// Header TR
		uint8_t m_tr;
		/// Header OS
		uint8_t m_os;
	};
}
