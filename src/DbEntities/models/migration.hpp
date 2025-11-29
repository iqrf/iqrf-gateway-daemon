/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

namespace iqrf::db::models {

/**
 * IQRF DB migration entity
 */
class Migration {
public:
  /**
   * Base constructor
   */
  Migration() = default;

  /**
   * Constructor without date
   * @param version Migration version
   * @param executedAt Migration executed timestamp
   */
  Migration(const std::string& version, const std::string& executedAt) : version(version), executedAt(executedAt) {}

  /**
   * Returns migration version
   * @return Migration version
   */
  const std::string& getVersion() {
    return version;
  }

  /**
   * Sets migration version
   * @param version Migration version
   */
  void setVersion(const std::string& version) {
    this->version = version;
  }

  /**
   * Returns migration executed timestamp
   * @return Migration executed timestamp
   */
  const std::string& getExecutedAt() {
    return executedAt;
  }

  /**
   * Sets migration executed timestamp
   * @param executedAt Migration executed timestamp
   */
  void setExecutedAt(const std::string &executedAt) {
    this->executedAt = executedAt;
  }

  /**
   * @brief Creates a Migration object from SQLite::Statement query result.
   *
   * @param stmt SQLiteCpp statement object
   * @return A new `Migration` constructed from query result.
   */
  static Migration fromResult(SQLite::Statement &stmt) {
    auto version = stmt.getColumn(0).getString();
    auto executedAt = stmt.getColumn(0).getString();
    return Migration(version, executedAt);
  }
private:
  /// Migration version
  std::string version;
  /// Migration executed timestamp
  std::string executedAt;
};

}

