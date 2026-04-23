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

#include "QuantityMemory.h"
#include "QuantityMemoryParser.h"
#include "PersistentQuantity.h"
#include "PersistentQuantityParser.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace iqrf::metadata {

  PersistentQuantity PersistentQuantityParser::parse(const nlohmann::json& doc) {
    std::optional<QuantityMemory> memory = std::nullopt;
    if (doc.contains("memory")) {
      memory = QuantityMemoryParser::parse(doc["memory"]);
    }

    return PersistentQuantity(
      parseValue<uint8_t>(doc, "quantity"),
      parseValue<uint32_t>(doc, "value"),
      parseValue<std::string>(doc, "description"),
      std::move(memory)
    );
  }

}
