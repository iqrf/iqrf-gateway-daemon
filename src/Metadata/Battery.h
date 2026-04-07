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

  class Battery {
   public:
    Battery(
      bool present,
      std::optional<std::string> type = std::nullopt,
      std::optional<double> changeThreshold = std::nullopt,
      std::optional<bool> conditioning = std::nullopt
    ): present_(present),
      type_(std::move(type)),
      changeThreshold_(std::move(changeThreshold)),
      conditioning_(std::move(conditioning)) {}

    bool present() const { return present_; }

    std::optional<std::string> type() const { return type_; }

    std::optional<double> changeThreshold() const { return changeThreshold_; }

    std::optional<bool> conditioning() const { return conditioning_; }
   private:
    bool present_;
    std::optional<std::string> type_;
    std::optional<double> changeThreshold_;
    std::optional<bool> conditioning_;
  };
}  // iqrf namespace
