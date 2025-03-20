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

namespace iqrf {

  // Decodes encoded IQRF Code
  class IqrfCodeDecoder {
  private:
    static std::basic_string<uint8_t> m_mid;
    static std::basic_string<uint8_t> m_ibk;
    static uint16_t m_hwpId;

  public:

    // decodes specified IQRF Code
    // must be called BEFORE getters of decoded values
    // throws exception, if some error occurred during decoding
    static void decode(const std::string& iqrfCode);

    // returns decoded MID
    static std::basic_string<uint8_t> getMid() { return m_mid; };

    // returns decoded IBK
    static std::basic_string<uint8_t> getIbk() { return m_ibk; };

    // returns decoded HWP id
    static uint16_t getHwpId() { return m_hwpId; };
  };
}
