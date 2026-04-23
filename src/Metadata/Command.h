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
#include <string>

namespace iqrf::metadata {

  /**
   * Command metadata
   *
   * This property was introduced in version 1.
   */
  class Command {
   public:
    /**
     * Constructs command metadata object
     *
     * @param value Command code / number
     * @param text Command description
     */
    Command(
      std::optional<uint8_t> value,
      std::optional<std::string> text
    ): value_(std::move(value)),
      text_(std::move(text)) {}

    /**
     * Get command code / number
     *
     * @return Command code / number
     */
    std::optional<uint8_t> value() const { return value_; }

    /**
     * Get command description
     *
     * The command text is primarily used for presentation layers.
     *
     * @return Command description
     */
    std::optional<std::string> text() const { return text_; }

   private:
    /// Command code / number
    std::optional<uint8_t> value_;
    /// Command description
    std::optional<std::string> text_;
  };
}  // iqrf namespace
