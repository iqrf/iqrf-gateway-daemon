/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#include <cstdint>
#include <string>

/// iqrf namespace
namespace iqrf {

	/// Code block encapsulating class
	class CodeBlock {
	public:
		/**
		 * Constructor
		 * @param code Code record
		 * @param startAddr Start address
		 * @param endAddr End address
		 */
		CodeBlock(const std::basic_string<uint8_t> &code, uint16_t startAddr, const uint16_t endAddr) {
			this->m_code = code;
			this->m_startAddr = startAddr;
			this->m_endAddr = endAddr;
		}

		/**
		 * Returns code record
		 * @return Code record
		 */
		const std::basic_string<uint8_t> &getCode() const {
			return m_code;
		}

		/**
		 * Returns start address
		 * @return Start address
		 */
		uint16_t getStartAddr() const {
			return m_startAddr;
		}

		/**
		 * Returns end address
		 * @return End address
		 */
		uint16_t getEndAddr() const {
			return m_endAddr;
		}

		/**
		 * Returns code record length
		 * @return Code record length
		 */
		uint16_t getLength() const {
			return m_code.size();
		}
	private:
		/// Code record
		std::basic_string<uint8_t> m_code;
		/// Start address
		uint16_t m_startAddr;
		/// End address
		uint16_t m_endAddr;
	};
}
