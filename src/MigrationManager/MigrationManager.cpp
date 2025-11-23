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

#include "MigrationManager.h"

#include "migration_repo.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace iqrf {

  MigrationManager::MigrationManager(const std::string& path): directory(path) {
    if (!std::filesystem::exists(directory)) {
      throw std::invalid_argument("Migration directory " + directory + " does not exist.");
    }
  }

  std::size_t MigrationManager::migrate(std::shared_ptr<SQLite::Database> connection) {
    auto available = getAvailableMigrations();
    std::size_t executed = 0;
    std::vector<std::string> migrationsToExecute;
    std::set<std::string> executedMigrations;
    if (connection->tableExists("migrations")) {
      db::repos::MigrationRepository migrationRepo(connection);
      executedMigrations = migrationRepo.getExecutedVersions();
    }

    for (auto &migration : available) {
      if (executedMigrations.count(migration) == 0) {
        migrationsToExecute.push_back(migration);
      }
    }

    try {
      for (const auto &migration : migrationsToExecute) {
        executeMigation(connection, directory + migration + ".sql");
        executed++;
      }
    } catch (const std::exception &e) {
      throw std::runtime_error(e.what());
    }
    return executed;
  }

  ///// Private methods /////

  std::vector<std::string> MigrationManager::getAvailableMigrations() {
    std::vector<std::string> migrations;
    for (const auto &file : std::filesystem::directory_iterator(directory)) {
      if (file.is_regular_file()) {
        migrations.push_back(file.path().stem());
      }
    }
    std::sort(migrations.begin(), migrations.end());
    return migrations;
  }

  void MigrationManager::executeMigation(std::shared_ptr<SQLite::Database> connection, const std::string& migration) {
    std::vector<std::string> statements;
    // try to access migration file
    std::ifstream migrationFile(migration);
    if (!migrationFile.is_open()) {
      throw std::runtime_error("Unable to read migration file: " + migration);
    }
    std::string line;
    std::stringstream statementStream;
    while (std::getline(migrationFile, line)) {
      // remove comments and empty lines
      if (line.empty() || line.rfind("--", 0) == 0) {
        continue;
      }
      statementStream << line;
    }
    // split into separate statements
    while (std::getline(statementStream, line, ';')) {
      statements.push_back(line);
    }
    // check for empty file
    if (statements.size() == 0) {
      throw std::runtime_error("Empty migration file: " + migration);
    }
    try {
    // execute migration statements
      for (auto &statement : statements) {
        connection->exec(statement);
      }
    } catch (const std::exception &e) {
      throw std::runtime_error(e.what());
    }
  }
}
