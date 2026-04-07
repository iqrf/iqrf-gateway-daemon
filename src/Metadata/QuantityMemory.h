/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
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
#include <optional>

namespace iqrf::metadata {

  class QuantityMemory {
   public:
    QuantityMemory(
      std::optional<uint8_t> type,
      std::optional<uint16_t> address
    ): type_(std::move(type)),
      address_(std::move(address)) {}

    std::optional<uint8_t> type() const { return type_; }

    std::optional<uint16_t> address() const { return address_; }

   private:
    std::optional<uint8_t> type_;
    std::optional<uint16_t> address_;
  };
}  // iqrf namespace
