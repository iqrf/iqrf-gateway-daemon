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

#include <cstdint>

#include <SQLiteCpp/SQLiteCpp.h>

namespace iqrf::db::models {

/**
 * IQRF DB Light entity
 */
class Light {
public:
  /**
   * Base constructor
   */
  Light() = default;

  /**
   * Constructor without ID
   * @param deviceId Device ID
   */
  Light(const uint32_t deviceId) : deviceId(deviceId) {};

  /**
   * Full constructor
   * @param id ID
   * @param deviceId Device ID
   */
  Light(const uint32_t id, const uint32_t deviceId) : id(id), deviceId(deviceId) {};

  /**
   * Returns Light ID
   * @return Light ID
   */
  uint32_t getId() const {
    return id;
  }

  /**
   * Sets Light ID
   * @param id Light ID
   */
  void setId(const uint32_t id) {
    this->id = id;
  }

  /**
   * Returns device ID
   * @return Device ID
   */
  uint32_t getDeviceId() const {
    return deviceId;
  }

  /**
   * Sets device ID
   * @param deviceId Device ID
   */
  void setDeviceId(const uint32_t deviceId) {
    this->deviceId = deviceId;
  }

  /**
   * @brief Creates a Light object from SQLite::Statement query result.
   *
   * @param stmt SQLiteCpp statement object
   * @return A new `Light` constructed from query result.
   */
  static Light fromResult(SQLite::Statement &stmt) {
    auto id = stmt.getColumn(0).getUInt();
    auto deviceId = stmt.getColumn(1).getUInt();
    return Light(id, deviceId);
  }
private:
  /// Light ID
  uint32_t id;
  /// Device ID
  uint32_t deviceId;
};

}
