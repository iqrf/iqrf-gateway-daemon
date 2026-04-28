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

#include "Accumulator.h"
#include "Battery.h"
#include "External.h"

#include <optional>
#include <utility>

namespace iqrf::metadata {

  /**
   * Power supply metadata
   */
  class PowerSupply {
   public:
    /**
     * Constructs power supply metadata
     *
     * @param mains Device is powered by mains power
     * @param accumulator Accumulator information
     * @param battery Battery information
     * @param external External power information
     * @param minVoltage Minimum operating voltage
     */
    PowerSupply(
      bool mains,
      Accumulator accumulator,
      Battery battery,
      std::optional<External> external,
      double minVoltage
    ): mains_(mains),
      accumulator_(std::move(accumulator)),
      battery_(std::move(battery)),
      external_(std::move(external)),
      minVoltage_(minVoltage) {}

    /**
     * Indicates whether device is powered by mains
     *
     * @return External power information
     */
    bool mains() const { return mains_; }

    /**
     * Get external power information
     *
     * As this is a version 0 property, it is expected
     * to be present until the metadata version is completely sunset.
     * Even if the device is not powered by an accumulator, the object is still available,
     * but with the `present` property set to `false` and all other properties `std::nullopt`.
     *
     * @return External power information
     */
    const Accumulator& accumulator() const { return accumulator_; }

    /**
     * Get external power information
     *
     * As this is a version 0 property, it is expected
     * to be present until the metadata version is completely sunset.
     * Even if the device is not powered by a battery, the object is still available,
     * but with the `present` property set to `false` and all other properties `std::nullopt`.
     *
     * @return External power information
     */
    const Battery& battery() const { return battery_; }

    /**
     * Get external power information
     *
     * This property was introduced in version 1.
     *
     * @return External power information
     */
    const std::optional<External>& external() const { return external_; }

    /**
     * Get minimum operating voltage
     *
     * @return Minimum operating voltage
     */
    double minVoltage() const { return minVoltage_; }

   private:
    /// Device is powered by mains power
    bool mains_;
    /// Accumulator information
    Accumulator accumulator_;
    /// Battery information
    Battery battery_;
    /// External power information
    std::optional<External> external_;
    /// Minimum operating voltage
    double minVoltage_;
  };
}  // iqrf namespace
