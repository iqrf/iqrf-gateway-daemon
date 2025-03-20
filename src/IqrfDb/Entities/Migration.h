/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <string>

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
   * Full constructor
   * @param version Migration version
   */
  Migration(const std::string &version) : version(version) {}

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
  void setVersion(const std::string &version) {
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
private:
  /// Migration version
  std::string version;
  /// Migration executed timestamp
  std::string executedAt;
};
