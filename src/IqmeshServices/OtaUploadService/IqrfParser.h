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

#include <fstream>
#include <list>

#include "CodeBlock.h"
#include "StringUtils.h"

#include "iqrf_header_parser.h"

namespace ihp = iqrf_header_parser;

/// iqrf namespace
namespace iqrf {
	/// IQRF plugin parser
	class IqrfParser {
	public:
		/**
		 * Constructor
		 * @param filename Path to IQRF plugin file
		 */
		IqrfParser(const std::string &filename) {
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
				m_contents.push_back(record);
			}
			file.close();
		}

		/**
		 * Parses IQRF plugin and returns flash data
		 * @return CodeBlock IQRF code blocks
		 */
		CodeBlock getFlashData() {
			return parse();
		}

		/**
		 * Returns plugin module compatibility information
		 * TR series in this case is TR family and requires conversion of device MCU and TR series values to TR family
		 * @return ihp::device::ModuleInfo Compatibility information
		 */
		ihp::device::ModuleInfo getHeaderModuleInfo() {
			ihp::device::ModuleInfo moduleInfo = ihp::device::ModuleInfo();
			moduleInfo.mcuType = m_mcu;
			moduleInfo.trSeries = m_tr;
			return moduleInfo;
		}

		/**
		 * Returns plugin OS compatibility information in string tokens
		 * IQRF plugin allows specification of OS versions and builds, and requires proper compatibility validation
		 * @return std::vector<std::string> OS compatibility tokens
		 */
		std::vector<std::string> getHeaderOs() {
			return m_os;
		}
	private:
		/**
		 * Parses IQRF plugin data to code blocks
		 * @return CodeBlock IQRF plugin code blocks
		 */
		CodeBlock parse() {
			std::basic_string<uint8_t> data;
			uint32_t cnt = 0;

			for (auto &record : m_contents) {
				cnt++;
				if (StringUtils::starsWith(record, "#")) {
					switch (cnt) {
					case 1:
						ihp::iqrf::parseMcuHeader(record, m_mcu, m_tr);
						break;
					case 2:
						ihp::iqrf::parseOsHeader(record, m_os);
						break;
					case 3: //date header
						break;
					case 4:
						if (ihp::iqrf::validPluginHeaderOs(record)) {
							throw std::logic_error("Regular ChangeOS plugin cannot be uploaded via OTA upload service.");
						}
						break;
					case 5: // separator
					default:
						if (!ihp::iqrf::isSeparator(record)) {
							throw std::logic_error("IQRF plugins should have only 4 programming headers and separator.");
						}
						break;
					}
					continue;
				}
				ihp::iqrf::validateData(record);
				for (uint32_t i = 0; i < ihp::iqrf::LINE_LENGTH; i += 2) {
					uint8_t byte = ihp::utils::hexStringToByte(record, i);
					data.push_back(byte);
				}
			}
			CodeBlock block(data, 0, data.size() - 1);
			return block;
		}

		/// List of code records
		std::list<std::string> m_contents;
		/// Header MCU
		uint8_t m_mcu;
		/// Header TR
		uint8_t m_tr;
		/// Header OS
		std::vector<std::string> m_os;
	};
}
