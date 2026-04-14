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

  /**
   * Persistent quantity memory metadata
   *
   * This property was introduced in version 1.
   */
  class QuantityMemory {
   public:
    /**
     * Constructs persistent quantity memory metadata
     *
     * @param type Memory type
     * @param address Address in memory
     */
    QuantityMemory(
      std::optional<uint8_t> type,
      std::optional<uint16_t> address
    ): type_(std::move(type)),
      address_(std::move(address)) {}

    /**
     * Get memory type
     *
     * Memory type value corresponds to memory peripheral number.
     *
     * @return Memory type
     */
    std::optional<uint8_t> type() const { return type_; }

    /**
     * Get address in memory
     *
     * @return Address in memory
     */
    std::optional<uint16_t> address() const { return address_; }

   private:
    /// Memory type
    std::optional<uint8_t> type_;
    /// Address in memory
    std::optional<uint16_t> address_;
  };
}  // iqrf namespace
