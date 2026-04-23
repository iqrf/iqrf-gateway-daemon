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

#include "HwpidVersions.h"
#include "HwpidVersionsParser.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <stdexcept>

namespace iqrf::metadata {

  HwpidVersions HwpidVersionsParser::parse(const nlohmann::json& doc) {
    checkRequired(doc);

    std::optional<uint32_t> max = std::nullopt;
    if (int64_t maxVal = doc["max"].get<int64_t>()) {
      if (maxVal >= 0) {
        max = static_cast<uint32_t>(maxVal);
      }
    }
    return HwpidVersions(
      doc["min"].get<uint32_t>(),
      std::move(max)
    );
  }

  void HwpidVersionsParser::checkRequired(const nlohmann::json& doc) {
    if (!doc.contains("min")) {
      throw std::invalid_argument("min field missing in hwpid versions definition.");
    }
    if (!doc.contains("max")) {
      throw std::invalid_argument("max field missing in hwpid versions definition.");
    }
  }

}
