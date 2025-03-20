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
#include <set>

namespace iqrf
{
  namespace embed
  {
    namespace coordinator
    {
      ////////////////
      class BondedDevices
      {
      protected:
        std::set<int> m_bondedDevices;

        BondedDevices()
        {}

      public:
        virtual ~BondedDevices()
        {}

        const std::set<int> & getBondedDevices() const { return m_bondedDevices; }

        bool isBonded(uint16_t nadr)
        {
          return m_bondedDevices.find(nadr) != m_bondedDevices.end();
        }
      };

      ////////////////
      class DiscoveredDevices
      {
      protected:
        std::set<int> m_discoveredDevices;

        DiscoveredDevices()
        {}

      public:
        virtual ~DiscoveredDevices()
        {}

        const std::set<int> & getDiscoveredDevices() const { return m_discoveredDevices; }

        // get more detailed data parsing
        bool isDiscovered(uint16_t nadr)
        {
          return m_discoveredDevices.find(nadr) != m_discoveredDevices.end();
        }
      };

    } //namespace coordinator
  } //namespace embed
} //namespace iqrf
