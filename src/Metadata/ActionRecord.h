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

#include "Command.h"
#include "RecordMemory.h"
#include <optional>
#include <vector>

namespace iqrf::metadata {

  /**
   * Action record metadata
   *
   * This property was introduced in version 1.
   */
  class ActionRecord {
   public:
    /**
     * Constructs action record metadata
     *
     * @param memory Definition of actions in memory
     * @param commands Available commands for invoking actions
     */
    ActionRecord(
      std::optional<RecordMemory> memory,
      std::vector<Command> commands
    ): memory_(std::move(memory)),
      commands_(std::move(commands)) {}

    /**
     * Get definition of actions in memory
     *
     * @return Definition of actions in memory
     */
    const std::optional<RecordMemory>& memory() const { return memory_; }

    /**
     * Get available commands for invoking actions
     *
     * @return Available commands for invoking actions
     */
    const std::vector<Command>& commands() const { return commands_; }

   private:
    /// Definition of actions in memory
    std::optional<RecordMemory> memory_;
    /// Available commands for invoking actions
    std::vector<Command> commands_;
  };
}  // iqrf namespace
