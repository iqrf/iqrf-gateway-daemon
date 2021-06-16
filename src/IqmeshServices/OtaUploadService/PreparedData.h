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

#include <vector>

namespace iqrf
{
  /// \class PreparedData
  /// \brief Holds data prepared for writing into external EEPROM and loading into
  /// MCU Flash memory on nodes.
  class PreparedData
  {
  public:
    typedef std::vector<std::basic_string<uint8_t>> Data;

    virtual Data getData() = 0;
    virtual uint16_t getChecksum() = 0;
    virtual uint16_t getLength() = 0;
  };
}