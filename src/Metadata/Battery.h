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

#include <optional>
#include <string>

namespace iqrf::metadata {

  /**
   * Battery metadata
   */
  class Battery {
   public:
    /**
     * Constructs battery metadata object
     *
     * @param present Battery present
     * @param type Battery type
     * @param changeThreshold Battery change charge threshold
     * @param conditioning Device performs battery conditioning
     */
    Battery(
      bool present,
      std::optional<std::string> type = std::nullopt,
      std::optional<double> changeThreshold = std::nullopt,
      std::optional<bool> conditioning = std::nullopt
    ): present_(present),
      type_(std::move(type)),
      changeThreshold_(std::move(changeThreshold)),
      conditioning_(std::move(conditioning)) {}

    /**
     * Get battery present state
     *
     * @return Battery present
     */
    bool present() const { return present_; }

    /**
     * Get battery type
     *
     * If battery is not present, type is `std::nullopt`.
     *
     * @return Battery present
     */
    std::optional<std::string> type() const { return type_; }

    /**
     * Get battery change charge threshold
     *
     * If battery is not present, changeThreshold is `std::nullopt`.
     *
     * @return Change threshold charge
     */
    std::optional<double> changeThreshold() const { return changeThreshold_; }

    /**
     * Get battery conditioning feature
     *
     * This property was introduced in version 1.
     *
     * If battery is not present, conditioning is `std::nullopt`.
     *
     * @return Device performs battery conditioning
     */
    std::optional<bool> conditioning() const { return conditioning_; }
   private:
    /// Battery present
    bool present_;
    /// Battery type
    std::optional<std::string> type_;
    /// Battery change charge threshold
    std::optional<double> changeThreshold_;
    /// Device performs battery conditioning
    std::optional<bool> conditioning_;
  };
}  // iqrf namespace
