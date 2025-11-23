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

#include <models/migration.hpp>
#include <repositories/base_repo.hpp>

#include <set>
#include <vector>

using iqrf::db::models::Migration;

namespace iqrf::db::repos {

/**
 * Migration repository
 */
class MigrationRepository : public BaseRepository {
public:
  using BaseRepository::BaseRepository;

  /**
   * @brief Lists all migration records
   *
   * @return Vector of deserialized `Migration` objects
   */
  std::vector<Migration> getAll() {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT version, executedAt
      FROM migrations;
      )"
    );
    std::vector<Migration> vec;
    while (stmt.executeStep()) {
      vec.emplace_back(Migration::fromResult(stmt));
    }
    return vec;
  }

  /**
   * @brief Finds and returns versions of executed migrations
   *
   * @return Set of migration versions
   */
  std::set<std::string> getExecutedVersions() {
    std::set<std::string> migrations;
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT version
      FROM migrations;
      )"
    );
    while(stmt.executeStep()) {
      migrations.insert(stmt.getColumn(0).getString());
    }
    return migrations;
  }
};

}
