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

#include <map>
#include <set>

#include <models/device_sensor.hpp>
#include <models/sensor.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::DeviceSensor;
using iqrf::db::models::Sensor;

namespace iqrf::db::repos {

/**
 * Device sensor repository
 */
class DeviceSensorRepository : public BaseRepository {
public:
  using BaseRepository::BaseRepository;

  /**
   * @brief Finds device sensor record by device address, sensor type and sensor index
   *
   * @param address Device address
   * @param type Sensor type
   * @param index Sensor index
   * @param frc Use FRC indexing
   *
   * @return Pointer to deserialized `DeviceSensor` object, or `nullptr` if record does not exist
   */
  std::unique_ptr<DeviceSensor> getByAddressTypeIndex(const uint8_t address, const uint8_t type, const uint8_t index, bool frc) {
    if (frc) {
      SQLite::Statement stmt(*m_db,
        R"(
        SELECT address, type, globalIndex, typeIndex, sensorId, value, updated, metadata
        FROM deviceSensor
        WHERE address = ? AND type = ?;
        )"
      );
      stmt.bind(1, address);
      stmt.bind(2, type);
      std::vector<DeviceSensor> vector;
      while (stmt.executeStep()) {
        vector.push_back(DeviceSensor::fromResult(stmt));
      }
      if (index >= vector.size()) {
        return nullptr;
      }
      return std::make_unique<DeviceSensor>(vector[index]);
    } else {
      SQLite::Statement stmt(*m_db,
        R"(
        SELECT address, type, globalIndex, typeIndex, sensorId, value, updated, metadata
        FROM deviceSensor
        WHERE address = ? AND type = ? and globalIndex = ?
        LIMIT 1;
        )"
      );
      stmt.bind(1, address);
      stmt.bind(2, type);
      stmt.bind(3, index);
      if (!stmt.executeStep()) {
        return nullptr;
      }
      return std::make_unique<DeviceSensor>(DeviceSensor::fromResult(stmt));
    }
  }

  /**
   * @brief Inserts a new device sensor record into database
   *
   * @param deviceSensor Device sensor object
   *
   * @throws `std::runtime_error` If the record cannot be inserted
   */
  void insert(DeviceSensor &deviceSensor) {
    SQLite::Statement stmt(*m_db,
      R"(
      INSERT INTO deviceSensor (address, type, globalIndex, typeIndex, sensorId, value, updated, metadata)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?);
      )"
    );
    stmt.bind(1, deviceSensor.getAddress());
    stmt.bind(2, deviceSensor.getType());
    stmt.bind(3, deviceSensor.getGlobalIndex());
    stmt.bind(4, deviceSensor.getTypeIndex());
    stmt.bind(5, deviceSensor.getSensorId());
    if (deviceSensor.getValue() == std::nullopt) {
      stmt.bind(6);
    } else {
      stmt.bind(6, deviceSensor.getValue().value());
    }
    if (deviceSensor.getUpdated() == nullptr) {
      stmt.bind(7);
    } else {
      stmt.bind(7, *deviceSensor.getUpdated());
    }
    if (deviceSensor.getMetadata() == nullptr) {
      stmt.bind(8);
    } else {
      stmt.bind(8, *deviceSensor.getMetadata());
    }
    try {
      stmt.exec();
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to insert new DeviceSensor entity",
          e.what()
        )
      );
    }
  }

  /**
   * @brief Updates existing device sensor record
   *
   * @param deviceSensor Device sensor object
   *
   * @throws `std::runtime_error` If the record cannot be updated
   */
  void update(DeviceSensor &deviceSensor) {
    SQLite::Statement stmt(*m_db,
      R"(
      UPDATE deviceSensor
      SET value = ?, updated = ?, metadata = ?
      WHERE address = ? AND type = ? AND globalIndex = ?;
      )"
    );
    if (deviceSensor.getValue() == std::nullopt) {
      stmt.bind(1);
    } else {
      stmt.bind(1, deviceSensor.getValue().value());
    }
    if (deviceSensor.getUpdated() == nullptr) {
      stmt.bind(2);
    } else {
      stmt.bind(2, *deviceSensor.getUpdated());
    }
    if (deviceSensor.getMetadata() == nullptr) {
      stmt.bind(3);
    } else {
      stmt.bind(3, *deviceSensor.getMetadata());
    }
    stmt.bind(4, deviceSensor.getAddress());
    stmt.bind(5, deviceSensor.getType());
    stmt.bind(6, deviceSensor.getGlobalIndex());
    try {
      stmt.exec();
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to update DeviceSensor entity at address " + std::to_string(deviceSensor.getAddress())
            + ", type " + std::to_string(deviceSensor.getType()) + ", index " + std::to_string(deviceSensor.getGlobalIndex()),
          e.what()
        )
      );
    }
  }

  /**
   * @brief Remove device sensor record by device address and sensor index
   *
   * @param addr Device address
   * @param index Sensor index
   */
  void removeByAddressIndex(const uint8_t addr, const uint8_t index) {
    SQLite::Statement stmt(*m_db,
      R"(
      DELETE FROM deviceSensor
      WHERE address = ? AND globalIndex = ?;
      )"
    );
    stmt.bind(1, addr);
    stmt.bind(2, index);
    stmt.exec();
  }

  /**
   * @brief Remove all device sensor records by device ID
   *
   * All sensors implemented by device are to be removed.
   *
   * @param addr Device address
   */
  void removeMultipleByAddress(const uint8_t addr) {
    SQLite::Statement stmt(*m_db,
      R"(
      DELETE FROM deviceSensor
      WHERE address = ?;
      )"
    );
    stmt.bind(1, addr);
    stmt.exec();
  }

  /**
   * @brief Remove device sensor records by address an select sensor indexes
   *
   * @param addr Device address
   * @param indexes Sensor indexes
   */
  void removeMultipleByAddressIndexes(const uint8_t addr, const std::vector<uint8_t>& indexes) {
    SQLite::Statement stmt(*m_db,
      R"(
      DELETE FROM deviceSensor
      WHERE address = ? AND globalIndex = ?;
      )"
    );
    stmt.bind(1, addr);
    for (const auto &index : indexes) {
      stmt.bind(2, index);
      stmt.exec();
      stmt.reset();
    }
  }

  /**
   * @brief Constructs and returns a map of device addresses, sensor indexes and sensor details
   *
   * @return Map of device addresses and vectors of sensor index and details pairs
   */
  std::map<uint8_t, std::vector<std::pair<uint8_t, Sensor>>> getDeviceAddressIndexSensorMap() {
    std::map<uint8_t, std::vector<std::pair<uint8_t, Sensor>>> map;
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT s.id, s.type, s.name, s.shortname, s.unit, s.decimals, s.frc2bit, s.frc1Byte, s.frc2Byte, s.frc4Byte,
          ds.address, ds.globalIndex
      FROM sensor as s
      INNER JOIN deviceSensor as ds
      ON ds.sensorId = s.id
      ORDER BY ds.address ASC, ds.globalIndex ASC;
      )"
    );
    std::vector<std::pair<uint8_t, Sensor>> vector;
    while (stmt.executeStep()) {
      const uint8_t addr = static_cast<uint8_t>(stmt.getColumn(10).getUInt());
      if (map.count(addr) == 0) {
        map.insert(
          std::make_pair(
            addr,
            std::vector<std::pair<uint8_t, Sensor>>{{
              static_cast<uint8_t>(stmt.getColumn(11).getUInt()),
              Sensor::fromResult(stmt)
            }}
          )
        );
      } else {
        map[addr].emplace_back(
          static_cast<uint8_t>(stmt.getColumn(11).getUInt()),
          Sensor::fromResult(stmt)
        );
      }
    }
    return map;
  }

  /**
   * @brief Constructs and returns a map of device addresses, device sensor and sensor details
   *
   * @return Map of device addresses and vectors of device sensor and sensor pairs
   */
  std::map<uint8_t, std::vector<std::pair<DeviceSensor, Sensor>>> getDeviceAddressSensorMap() {
    std::map<uint8_t, std::vector<std::pair<DeviceSensor, Sensor>>> map;
    SQLite::Statement dsStmt(*m_db,
      R"(
      SELECT address, type, globalIndex, typeIndex, sensorId, value, updated, metadata
      FROM deviceSensor
      ORDER BY address ASC, globalIndex ASC;
      )"
    );
    while (dsStmt.executeStep()) {
      auto deviceSensor = DeviceSensor::fromResult(dsStmt);
      SQLite::Statement sStmt(*m_db,
        R"(
        SELECT id, type, name, shortname, unit, decimals, frc2bit, frc1Byte, frc2Byte, frc4Byte
        FROM sensor
        WHERE id = ?
        LIMIT 1;
        )"
      );
      sStmt.bind(1, deviceSensor.getSensorId());
      if (!sStmt.executeStep()) {
        continue;
      }
      const uint8_t addr = deviceSensor.getAddress();
      if (map.count(addr) == 0) {
        map.insert(
          std::make_pair(
            addr,
            std::vector<std::pair<DeviceSensor, Sensor>>{{
              deviceSensor,
              Sensor::fromResult(sStmt)
            }}
          )
        );
      } else {
        map[addr].emplace_back(
          deviceSensor,
          Sensor::fromResult(sStmt)
        );
      }
    }
    return map;
  }

  /**
   * @brief Constructs and returns a map of sensor types, device addresses and sensor indexes
   *
   * @return Map of sensor types and vectors of device address and index pairs
   */
  std::unordered_map<uint8_t, std::vector<std::pair<uint8_t, uint8_t>>> getSensorTypeAddressIndexMap() {
    std::unordered_map<uint8_t, std::vector<std::pair<uint8_t, uint8_t>>> map;
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT address, type, typeIndex
      FROM deviceSensor
      ORDER BY address ASC, globalIndex ASC;
      )"
    );
    while (stmt.executeStep()) {
      uint8_t type = static_cast<uint8_t>(stmt.getColumn(1).getUInt());
      map[type].emplace_back(
        static_cast<uint8_t>(stmt.getColumn(0).getUInt()),
        static_cast<uint8_t>(stmt.getColumn(2).getUInt())
      );
    }
    return map;
  }

  /**
   * @brief Finds a device sensor record and returns the global index
   *
   * Sensor type index is different from global index, global index is index of sensor of any type
   * implemented by device. Type index is index of sensor of specific type implemented by device.
   *
   * If for example a device implements temperature and humidity sensors,
   * their global index is 0 and 1 respectively. Type index of each of those sensors is 0 as they
   * are the first implemented sensors of their respective types by the device.
   *
   * @param address Device address
   * @param type Sensor type
   * @param typeIndex Sensor type index
   *
   * @return Optional value container, global index if record exists, `std::nullopt` otherwise
   */
  std::optional<uint8_t> getGlobalSensorIndex(const uint8_t address, const uint8_t type, const uint8_t typeIndex) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT globalIndex
      FROM deviceSensor
      WHERE address = ? AND type = ? AND typeIndex = ?
      LIMIT 1;
      )"
    );
    stmt.bind(1, address);
    stmt.bind(2, type);
    stmt.bind(3, typeIndex);
    if (!stmt.executeStep()) {
      return std::nullopt;
    }
    return static_cast<uint8_t>(stmt.getColumn(0).getUInt());
  }

  /**
   * @brief Constructs and returns map of HWPIDs and device addresses, for each device implementing specific sensor
   *
   * HWPID represents a product type, multiple devices can be of the same product.
   *
   * @param type Sensor type
   * @return Map of HWPIDs and set of device addresses
   */
  std::map<uint16_t, std::set<uint8_t>> getHwpidAddressesMap(const uint8_t type) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT d.address, p.hwpid
      FROM product as p
      INNER JOIN device as d ON d.productId = p.id
      INNER JOIN deviceSensor as ds ON ds.address = d.address
      WHERE ds.type = ?
      GROUP BY d.address;
      )"
    );
    stmt.bind(1, type);
    std::map<uint16_t, std::set<uint8_t>> map;
    while (stmt.executeStep()) {
      auto addr = static_cast<uint8_t>(stmt.getColumn(0).getUInt());
      auto hwpid = static_cast<uint16_t>(stmt.getColumn(1).getUInt());
      if (map.count(hwpid) == 0) {
        map.insert(
          std::make_pair(
            hwpid,
            std::set<uint8_t>{addr}
          )
        );
      } else {
        map[hwpid].insert(addr);
      }
    }
    return map;
  }

  /**
   * @brief Finds device sensor records and returns their global indexs and sensor IDs on device
   *
   * @param addr Device address
   * @return Map of global indexes and corresponding sensor IDs
   */
  std::map<uint8_t, uint32_t> getGlobalIndexSensorIdMap(const uint8_t addr) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT globalIndex, sensorId
      FROM deviceSensor
      WHERE address = ?;
      )"
    );
    stmt.bind(1, addr);
    std::map<uint8_t, uint32_t> map;
    while (stmt.executeStep()) {
      map.insert_or_assign(
        static_cast<uint8_t>(stmt.getColumn(0).getUInt()),
        stmt.getColumn(1).getUInt()
      );
    }
    return map;
  }

  /**
   * @brief Checks if device implements sensor standard
   *
   * @param address Device address
   * @return `true` if Device implements sensors, `false` otherwise
   */
  bool deviceHasSensors(const uint8_t address) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT COUNT(*)
      FROM deviceSensor
      WHERE address = ?;
      )"
    );
    stmt.bind(1, address);
    if (!stmt.executeStep()) {
      return false;
    }
    return stmt.getColumn(0).getUInt() > 0;
  }
};

}
