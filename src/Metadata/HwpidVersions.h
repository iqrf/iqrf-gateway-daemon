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
   * HWPID versions metadata
   */
  class HwpidVersions {
   public:
    /**
     * Constructs HWPID versions metadata
     *
     * @param min Minimum HWPID version
     * @param max Maximum HWPID version
     */
    HwpidVersions(
      uint32_t min,
      std::optional<uint32_t> max = std::nullopt
    ): min_(min),
      max_(std::move(max)) {}

    /**
     * Get minimum HWPID version
     *
     * @return Minimum HWPID version
     */
    uint32_t min() const { return min_; }

    /**
     * Get maximum HWPID version
     *
     * Due to compatibility and parsing reasons
     * with other systems, value -1 in metadata represents
     * that there is no maximum version specified for the
     * metadata profile and applies to all versions
     * higher than min.
     *
     * If maximum HWPID version is not specified,
     * this returns null.
     *
     * @return Maximum HWPID version
     */
    std::optional<uint32_t> max() const { return max_; }

   private:
    /// Minimum HWPID version
    uint32_t min_;
    /// Maximum HWPID version
    std::optional<uint32_t> max_;
  };
}  // iqrf namespace
