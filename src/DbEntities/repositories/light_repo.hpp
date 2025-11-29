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

#include <set>

#include <models/light.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::Light;

namespace iqrf::db::repos {

/**
 * Light repository
 */
class LightRepository : public BaseRepository {
public:
  using BaseRepository::BaseRepository;

  /**
   * @brief Finds light record by ID
   *
   * @param id Record ID
   * @return Pointer to deserialized `Light` object, or `nullptr` if record does not exist
   */
  std::unique_ptr<Light> get(const uint32_t id) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, deviceId
      FROM light
      WHERE id = ?
      LIMIT 1;
      )"
    );
    stmt.bind(1, id);
    if (!stmt.executeStep()) {
      return nullptr;
    }
    return std::make_unique<Light>(Light::fromResult(stmt));
  }

  /**
   * @brief Lists all light records
   *
   * @return Vector of deserialized `Light` objects
   */
  std::vector<Light> getAll() {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, deviceId
      FROM light;
      )"
    );
    std::vector<Light> vec;
    while (stmt.executeStep()) {
      vec.emplace_back(Light::fromResult(stmt));
    }
    return vec;
  }

  /**
   * @brief Finds light record by deviceId
   *
   * @param deviceId Device ID
   * @return Pointer to deserialized `Light` object, or `nullptr` if record does not exist
   */
  std::unique_ptr<Light> getByDeviceId(const uint32_t deviceId) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, deviceId
      FROM light
      WHERE deviceId = ?
      LIMIT 1;
      )"
    );
    stmt.bind(1, deviceId);
    if (!stmt.executeStep()) {
      return nullptr;
    }
    return std::make_unique<Light>(Light::fromResult(stmt));
  }

  /**
   * @brief Inserts new light record into database
   *
   * @param light Light object
   * @return ID of inserted record
   *
   * @throws `std::runtime_error` If the record cannot be inserted
   */
  uint32_t insert(Light &light) {
    SQLite::Statement stmt(*m_db,
      R"(
      INSERT INTO light (deviceId)
      VALUES (?);
      )"
    );
    stmt.bind(1, light.getDeviceId());
    try {
      stmt.exec();
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to insert new Light entity",
          e.what()
        )
      );
    }
    return m_db->getLastInsertRowid();
  }

  /**
   * @brief Updates existing light record
   *
   * @param light Light object
   *
   * @throws `std::runtime_error` If the record cannot be updated
   */
  void update(Light &light) {
    SQLite::Statement stmt(*m_db,
      R"(
      UPDATE light
      SET deviceId = ?
      WHERE id = ?;
      )"
    );
    stmt.bind(1, light.getDeviceId());
    stmt.bind(2, light.getId());
    try {
      stmt.exec();
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to update driver entity ID " + std::to_string(light.getId()),
          e.what()
        )
      );
    }
  }

  /**
   * @brief Removes existing light record by ID
   *
   * @param id Record ID
   */
  void remove(const uint32_t id) {
    SQLite::Statement stmt(*m_db,
      R"(
      DELETE FROM light
      WHERE id = ?;
      )"
    );
    stmt.bind(1, id);
    stmt.exec();
  }

  /**
   * @brief Removes existing light record by device ID
   *
   * @param deviceId Device ID
   */
  void removeByDeviceId(const uint32_t deviceId) {
    SQLite::Statement stmt(*m_db,
      R"(
      DELETE FROM light
      WHERE deviceId = ?;
      )"
    );
    stmt.bind(1, deviceId);
    stmt.exec();
  }

  /**
   * @brief Finds and returns addresses of devices implementing light standard
   *
   * @return Set of device addresses
   */
  std::set<uint8_t> getAddresses() {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT d.address
      FROM light as l
      INNER JOIN device as d ON d.id = l.deviceId;
      )"
    );
    std::set<uint8_t> addrs;
    while(stmt.executeStep()) {
      addrs.insert(static_cast<uint8_t>(stmt.getColumn(0).getUInt()));
    }
    return addrs;
  }
};

}
