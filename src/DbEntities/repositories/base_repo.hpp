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

#include <memory>

#include <SQLiteCpp/SQLiteCpp.h>

namespace iqrf::db::repos {

/**
 * Base repository class
 */
class BaseRepository {
public:
  /**
   * Constructor
   *
   * @param db Shared pointer to database connection
   */
  explicit BaseRepository(std::shared_ptr<SQLite::Database> db) : m_db(db) {}

  /**
   * Destructor
   */
  ~BaseRepository() = default;
protected:
  /**
   * @brief Formats error message with exception text
   *
   * @param description Error description text
   * @param exmsg Exception text
   */
  std::string formatErrorMessage(std::string description, std::string exmsg) {
    std::ostringstream oss;
    oss << description << ": " << exmsg;
    return oss.str();
  }

  /// Database connection container
  std::shared_ptr<SQLite::Database> m_db;
};

}
