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
#include <vector>

namespace iqrf
{
  namespace embed
  {
    namespace eeeprom
    {
      ////////////////
      class Read
      {
      protected:
        //params
        uint16_t m_address;
        uint8_t m_len;

        //response
        std::vector<int> m_pdata;

        Read()
        {}

        Read(uint16_t address, uint8_t len)
          : m_address(address)
          , m_len(len)
        {}

      public:
        virtual ~Read()
        {}

        // get read data
        const std::vector<int> & getPdata() const { return m_pdata; }
      };

    } //namespace eeeprom
  } //namespace embed
} //namespace iqrf
