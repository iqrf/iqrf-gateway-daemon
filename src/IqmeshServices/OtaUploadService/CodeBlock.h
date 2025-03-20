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
			return m_code.length();
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
