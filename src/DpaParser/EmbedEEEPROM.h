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
