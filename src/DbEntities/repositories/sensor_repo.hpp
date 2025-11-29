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

#include <models/sensor.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::Sensor;

namespace iqrf::db::repos {

/**
 * Sensor repository
 */
class SensorRepository : public BaseRepository {
public:
  using BaseRepository::BaseRepository;

  /**
   * @brief Finds sensor record by ID
   *
   * @param id Record ID
   * @return Pointer to deserialized `Sensor` object, or `nullptr` if record does not exist
   */
  std::unique_ptr<Sensor> get(const uint32_t id) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, type, name, shortname, unit, decimals, frc2bit, frc1Byte, frc2Byte, frc4Byte
      FROM sensor
      WHERE id = ?
      LIMIT 1;
      )"
    );
    stmt.bind(1, id);
    if (!stmt.executeStep()) {
      return nullptr;
    }
    return std::make_unique<Sensor>(Sensor::fromResult(stmt));
  }

  /**
   * @brief Finds sensor record by type and name
   *
   * @param type Sensor type
   * @param name Quantity name
   * @return Pointer to deserialized `Sensor` object, or `nullptr` if record does not exist
   */
  std::unique_ptr<Sensor> getByTypeName(const uint8_t type, const std::string& name) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, type, name, shortname, unit, decimals, frc2bit, frc1Byte, frc2Byte, frc4Byte
      FROM sensor
      WHERE type = ? AND name = ?
      LIMIT 1;
      )"
    );
    stmt.bind(1, type);
    stmt.bind(2, name);
    if (!stmt.executeStep()) {
      return nullptr;
    }
    return std::make_unique<Sensor>(Sensor::fromResult(stmt));
  }

  /**
   * @brief Finds sensor record by device address, sensor index and type
   *
   * @param address Device address
   * @param index Sensor index
   * @param type Sensor type
   * @return Pointer to deserialized `Sensor` object, or `nullptr` if record does not exist
   */
  std::unique_ptr<Sensor> getByAddressIndexType(const uint8_t address, const uint8_t index, const uint8_t type) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT s.id, s.type, s.name, s.shortname, s.unit, s.decimals, s.frc2bit, s.frc1Byte, s.frc2Byte, s.frc4Byte
      FROM sensor as s
      INNER JOIN deviceSensor as ds
      ON ds.sensorId = s.id
      WHERE ds.address = ? and ds.globalIndex = ? and ds.type = ?
      LIMIT 1;
      )"
    );
    stmt.bind(1, address);
    stmt.bind(2, index);
    stmt.bind(3, type);
    if (!stmt.executeStep()) {
      return nullptr;
    }
    return std::make_unique<Sensor>(Sensor::fromResult(stmt));
  }

  /**
   * @brief Inserts new sensor record into database
   *
   * @param sensor Sensor object
   * @return ID of inserted record
   *
   * @throws `std::runtime_error` If the record cannot be inserted
   */
  uint32_t insert(Sensor &sensor) {
    SQLite::Statement stmt(*m_db,
      R"(
      INSERT INTO sensor (type, name, shortname, unit, decimals, frc2bit, frc1Byte, frc2Byte, frc4Byte)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
      )"
    );
    stmt.bind(1, sensor.getType());
    stmt.bind(2, sensor.getName());
    stmt.bind(3, sensor.getShortname());
    stmt.bind(4, sensor.getUnit());
    stmt.bind(5, sensor.getDecimals());
    stmt.bind(6, sensor.hasFrc2Bit());
    stmt.bind(7, sensor.hasFrcByte());
    stmt.bind(8, sensor.hasFrc2Byte());
    stmt.bind(9, sensor.hasFrc4Byte());
    try {
      stmt.exec();
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to insert new Sensor entity",
          e.what()
        )
      );
    }
    return m_db->getLastInsertRowid();
  }

  /**
   * @brief Constructs and returns map of sensors implemented by device
   *
   * @param address Device address
   * @return Map of sensor indexes and sensor objects
   */
  std::map<uint8_t, Sensor> getDeviceSensorIndexMap(const uint8_t address) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT s.id, s.type, s.name, s.shortname, s.unit, s.decimals, s.frc2bit, s.frc1Byte, s.frc2Byte, s.frc4Byte
      FROM sensor as s
      INNER JOIN deviceSensor as ds
      ON ds.sensorId = s.id
      WHERE ds.address = ?
      ORDER BY ds.globalIndex;
      )"
    );
    stmt.bind(1, address);
    std::map<uint8_t, Sensor> map;
    size_t i = 0;
    while (stmt.executeStep()) {
      map.insert(
        std::make_pair(
          i++,
          Sensor::fromResult(stmt)
        )
      );
    }
    return map;
  }

  /**
   * @brief Constructs and returns map of device sensor indexes and sensor IDs on device
   *
   * @param address Device address
   * @return Map of global indexes and sensor IDs
   */
  std::map<uint8_t, uint32_t> getDeviceSensorIdIndexMap(const uint8_t address) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT ds.globalIndex, s.id
      FROM sensor as s
      INNER JOIN deviceSensor as ds
      ON ds.sensorId = s.id
      WHERE ds.address = ?
      ORDER BY ds.globalIndex;
      )"
    );
    stmt.bind(1, address);
    std::map<uint8_t, uint32_t> map;
    while (stmt.executeStep()) {
      map.insert(
        std::make_pair(
          static_cast<uint8_t>(stmt.getColumn(0).getUInt()),
          stmt.getColumn(1).getUInt()
        )
      );
    }
    return map;
  }
};

}
