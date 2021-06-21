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
