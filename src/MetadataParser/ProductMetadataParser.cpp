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

#include "Metadata.h"
#include "MetadataParser.h"
#include "ProductMetadata.h"
#include "ProductMetadataParser.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

namespace iqrf::metadata {

  ProductMetadata ProductMetadataParser::parse(const nlohmann::json& doc) {
    checkRequired(doc);

    uint32_t version = doc["version"].get<uint32_t>();
    if (doc.contains("version")) {
      version = doc["version"].get<uint32_t>();
    }

    using ParseHandler = std::function<Metadata(const nlohmann::json&)>;
    ParseHandler handler = nullptr;

    switch (version) {
      case 0:
        handler = MetadataParser::parseV0;
        break;
      case 1:
        handler = MetadataParser::parseV1;
        break;
      default:
        throw std::runtime_error("Unsupported metadata version.");
    }

    std::vector<Metadata> profiles = {};
    for (const auto& item : doc["profiles"]) {
      profiles.push_back(handler(item));
    }

    return ProductMetadata(
      version,
      std::move(profiles)
    );
  }

  void ProductMetadataParser::checkRequired(const nlohmann::json& doc) {
    if (!doc.contains("version")) {
      throw std::invalid_argument("version field missing in product metadata definition.");
    }
    if (!doc.contains("profiles")) {
      throw std::invalid_argument("profiles definitions missing in product metadata definition.");
    }
  }
}  // iqrf namespace
