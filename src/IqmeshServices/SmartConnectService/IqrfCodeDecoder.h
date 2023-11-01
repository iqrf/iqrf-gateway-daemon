/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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
