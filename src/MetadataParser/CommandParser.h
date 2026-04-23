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
#include "Command.h"

#include <nlohmann/json.hpp>

namespace iqrf::metadata {

  /**
   * Command metadata parser
   */
  class CommandParser : public BaseParser {
   public:
    /**
     * Parse `nlohmann::json` document containing command metadata
     * and return `Command` object.
     *
     * @param doc `nlohmann::json` document
     * @return `Command` Parsed command metadata
     */
    static Command parse(const nlohmann::json& doc);
  };
}  // iqrf::metadata namespace
