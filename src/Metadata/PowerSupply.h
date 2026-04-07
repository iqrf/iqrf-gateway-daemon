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

  class PowerSupply {
   public:
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

    bool mains() const { return mains_; }

    const Accumulator& accumulator() const { return accumulator_; }

    const Battery& battery() const { return battery_; }

    const std::optional<External> external() const { return external_; }

    double minVoltage() const { return minVoltage_; }

   private:
    bool mains_;
    Accumulator accumulator_;
    Battery battery_;
    std::optional<External> external_;
    double minVoltage_;
  };
}  // iqrf namespace
