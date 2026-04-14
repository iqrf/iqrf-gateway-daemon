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

#include "BaseParser.h"
#include "ProductMetadata.h"

#include <nlohmann/json.hpp>

#define MIN_METADATA_VERSION 0
#define MAX_METADATA_VERSION 1

namespace iqrf::metadata {

  /**
   * Product metadata parser
   */
  class ProductMetadataParser : public BaseParser {
   public:
    /**
     * Parse `nlohmann::json` document containing product metadata
     * and return `ProductMetadata` object.
     *
     * The method first retrieves metadata version and internally parses
     * the metadata using parsing methods for the specific versions,
     * if the metadata version is supported by the parser.
     *
     * @param doc `nlohmann::json` document
     * @return `ProductMetadata` Parsed product metadata
     * @throws `std::runtime_error` Thrown if the parsed metadata version is not supported
     */
    static ProductMetadata parse(const nlohmann::json& doc);
   private:
    /**
     * Checks that passed document contains required properties
     *
     * Since metadata version 0 is still supported, the original properties
     * are required to be present in metadata with at least falsy values (if device
     * does not have certain properties).
     *
     * The required properties are: version, profiles
     *
     * @param doc `nlohmann::json` document
     * @throws `std::invalid_argument` Thrown if either of the required properties is not present
     */
    static void checkRequired(const nlohmann::json& doc);
  };
}
