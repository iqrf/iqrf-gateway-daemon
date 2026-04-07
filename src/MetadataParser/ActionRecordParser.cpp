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

#include "ActionRecord.h"
#include "ActionRecordParser.h"
#include "Command.h"
#include "CommandParser.h"
#include "RecordMemory.h"
#include "RecordMemoryParser.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <vector>

namespace iqrf::metadata {

  ActionRecord ActionRecordParser::parse(const nlohmann::json& doc) {
    std::optional<RecordMemory> memory = std::nullopt;
    if (doc.contains("memory")) {
      memory = RecordMemoryParser::parse(doc["memory"]);
    }
    std::vector<Command> commands = {};
    if (doc.contains("commands")) {
      for (const auto& item : doc["commands"]) {
        commands.push_back(CommandParser::parse(item));
      }
    }

    return ActionRecord(
      std::move(memory),
      std::move(commands)
    );
  }

}
