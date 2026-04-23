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

#include "AccumulatorParser.h"
#include "BatteryParser.h"
#include "External.h"
#include "ExternalParser.h"
#include "PowerSupply.h"
#include "PowerSupplyParser.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <stdexcept>
#include <utility>

namespace iqrf::metadata {

  PowerSupply PowerSupplyParser::parseV0(const nlohmann::json& doc) {
    checkRequired(doc);

    return PowerSupply(
      doc["mains"].get<bool>(),
      AccumulatorParser::parse(doc["accumulator"]),
      BatteryParser::parseV0(doc["battery"]),
      std::nullopt,
      doc["minVoltage"].get<double>()
    );
  }

  PowerSupply PowerSupplyParser::parseV1(const nlohmann::json& doc) {
    checkRequired(doc);

    std::optional<External> external = std::nullopt;
    if (doc.contains("external")) {
      external = ExternalParser::parse(doc["external"]);
    }

    return PowerSupply(
      doc["mains"].get<bool>(),
      AccumulatorParser::parse(doc["accumulator"]),
      BatteryParser::parseV1(doc["battery"]),
      std::move(external),
      doc["minVoltage"].get<double>()
    );
  }

  void PowerSupplyParser::checkRequired(const nlohmann::json& doc) {
    if (!doc.contains("mains")) {
      throw std::invalid_argument("mains field missing in power supply definition.");
    }
    if (!doc.contains("accumulator")) {
      throw std::invalid_argument("accumulator definition missing in power supply definition.");
    }
    if (!doc.contains("battery")) {
      throw std::invalid_argument("battery definition missing in power supply definition.");
    }
    if (!doc.contains("minVoltage")) {
      throw std::invalid_argument("minVoltage field missing in power supply definition.");
    }
  }

}
