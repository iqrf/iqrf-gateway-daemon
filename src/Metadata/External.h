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
   * External power metadata
   *
   * This property was introduced in version 1.
   */
  class External {
   public:
    /**
     * Constructs external power metadata object
     *
     * @param type External power type
     * @param nominalVoltage Nominal voltage
     * @param minVoltage Minimum voltage
     * @param maxVoltage Maximum voltage
     */
    External(
      std::optional<std::string> type,
      std::optional<double> nominalVoltage,
      std::optional<double> minVoltage,
      std::optional<double> maxVoltage
    ): type_(std::move(type)),
      nominalVoltage_(std::move(nominalVoltage)),
      minVoltage_(std::move(minVoltage)),
      maxVoltage_(std::move(maxVoltage)) {}

    /**
     * Get external power type
     *
     * @return External power type
     */
    std::optional<std::string> type() const { return type_; }

    /**
     * Get nominal voltage
     *
     * @return Nominal voltage
     */
    std::optional<double> nominalVoltage() const { return nominalVoltage_; }

    /**
     * Get minimum voltage
     *
     * @return Minimum voltage
     */
    std::optional<double> minVoltage() const { return minVoltage_; }

    /**
     * Get maximum voltage
     *
     * @return Maximum voltage
     */
    std::optional<double> maxVoltage() const { return maxVoltage_; }

   private:
    /// External power type
    std::optional<std::string> type_;
    /// Nominal voltage
    std::optional<double> nominalVoltage_;
    /// Minimum voltage
    std::optional<double> minVoltage_;
    /// Maximum voltage
    std::optional<double> maxVoltage_;
  };
}  // iqrf namespace
