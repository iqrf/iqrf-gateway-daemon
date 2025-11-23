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

#include <SQLiteCpp/SQLiteCpp.h>

#include <memory>
#include <string>
#include <vector>

namespace iqrf {

  class MigrationManager {
  public:
    /**
     * Parameterized constructor
     *
     * @param path Path to directory containing migrations
     */
    explicit MigrationManager(const std::string& path);

    MigrationManager(const MigrationManager&) = default;
    MigrationManager& operator=(const MigrationManager&) = default;
    MigrationManager(MigrationManager&&) = default;
    MigrationManager& operator=(MigrationManager&&) = default;

    /**
     * @brief Performs migration to latest schema version
     *
     * @param connection SQLite database connection
     * @return `std::size_t` Number of executed migrations
     */
    std::size_t migrate(std::shared_ptr<SQLite::Database> connection);

  private:
    /**
     * Finds and returns all available migrations files
     *
     * @return `std::vector<std::string>` Vector of migration file names
     */
    std::vector<std::string> getAvailableMigrations();

    /**
     * @brief Executes migration on database connection
     *
     * @param connection Database connection
     * @param migration Path to migration file
     *
     * @throws `std::runtime_error` If migration file does not exist, cannot be read, is empty, or contains invalid statements
     */
    void executeMigation(std::shared_ptr<SQLite::Database> connection, const std::string& migration);

    /**
     * @brief Path to directory containnig migrations
     */
    std::string directory;
  };
}
