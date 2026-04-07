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

#include "Accumulator.h"
#include "AccumulatorParser.h"

#include <nlohmann/json.hpp>

#include <stdexcept>

namespace iqrf::metadata {

  Accumulator AccumulatorParser::parse(const nlohmann::json& doc) {
    checkRequired(doc);

    return Accumulator(
      doc["present"].get<bool>(),
      parseValue<std::string>(doc, "type"),
      parseValue<double>(doc, "lowLevel")
    );
  }

  void AccumulatorParser::checkRequired(const nlohmann::json& doc) {
    if (!doc.contains("present")) {
      throw std::invalid_argument("present field missing in accumulator definition.");
    }
    if (!doc.contains("type")) {
      throw std::invalid_argument("type field missing in accumulator definition.");
    }
    if (!doc.contains("lowLevel")) {
      throw std::invalid_argument("lowLevel field missing in accumulator definition.");
    }
  }

}
