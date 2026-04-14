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
   * Action record memory metadata
   *
   * This property was introduced in version 1.
   */
  class RecordMemory {
   public:
    /**
     * Constructs action record memory metadata
     *
     * @param type Memory type
     * @param address Address in memory
     * @param size Size of memory block designated for actions
     */
    RecordMemory(
      std::optional<uint8_t> type,
      std::optional<uint16_t> address,
      std::optional<uint16_t> size
    ): type_(std::move(type)),
      address_(std::move(address)),
      size_(std::move(size)) {}

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

    /**
     * Get size of memory block designated for action
     *
     * @return Size of memory block designated for action
     */
    std::optional<uint16_t> size() const { return size_; }

   private:
    /// Memory type
    std::optional<uint8_t> type_;
    /// Address in memory
    std::optional<uint16_t> address_;
    /// Size of memory block designed for actions
    std::optional<uint16_t> size_;
  };
}  // iqrf namespace
