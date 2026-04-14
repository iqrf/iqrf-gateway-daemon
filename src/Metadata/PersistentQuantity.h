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

#include "QuantityMemory.h"

#include <cstdint>
#include <string>

namespace iqrf::metadata {

  /**
   * Persistent quantity metadata
   *
   * This property was introduced in version 1.
   */
  class PersistentQuantity {
   public:
    /**
     * Constructs persistent quantity metadata
     *
     * @param quantity Quantity type (IQRF Sensor)
     * @param value Value stored
     * @param description Description
     * @param quantityMemory Memory information
     */
    PersistentQuantity(
      std::optional<uint8_t> quantity,
      std::optional<uint32_t> value,
      std::optional<std::string> description,
      std::optional<QuantityMemory> quantityMemory
    ): quantity_(std::move(quantity)),
      value_(std::move(value)),
      description_(std::move(description)),
      quantityMemory_(std::move(quantityMemory)) {}

    /**
     * Get quantity type
     *
     * Quantity type is the IQRF Sensor standard quantity type value.
     *
     * @return Quantity type
     */
    std::optional<uint8_t> quantity() const { return quantity_; }

    /**
     * Get stored value
     *
     * Value is stored as an integer, before any conversion
     * according to the IQRF Sensor standard is performed.
     *
     * @return Stored value
     */
    std::optional<uint32_t> value() const { return value_; }

    /**
     * Get description
     *
     * Description is primarily used for presentation layers.
     *
     * @return Description
     */
    const std::optional<std::string>& description() const { return description_; }

    /**
     * Get memory information for quantity
     *
     * Quantity memory information contains the memory type and address in memory
     * where the value is stored.
     *
     * @return Memory information
     */
    const std::optional<QuantityMemory>& quantityMemory() const { return quantityMemory_; }

   private:
    /// Quantity type (IQRF Sensor)
    std::optional<uint8_t> quantity_;
    /// Value stored
    std::optional<uint32_t> value_;
    /// Description
    std::optional<std::string> description_;
    /// Memory information
    std::optional<QuantityMemory> quantityMemory_;
  };
}  // iqrf namespace
