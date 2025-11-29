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

#include <models/binary_output.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::BinaryOutput;

namespace iqrf::db::repos {

/**
 * BinaryOutput repository
 */
class BinaryOutputRepository : public BaseRepository {
public:
  using BaseRepository::BaseRepository;

  /**
   * @brief Finds binary output record by ID
   *
   * @param id Record ID
   * @return Pointer to deserialized `BinaryOutput` object, or `nullptr` if record does not exist
   */
  std::unique_ptr<BinaryOutput> get(const uint32_t id) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, deviceId, count
      FROM bo
      WHERE id = ?
      LIMIT 1;
      )"
    );
    stmt.bind(1, id);
    if (!stmt.executeStep()) {
      return nullptr;
    }
    return std::make_unique<BinaryOutput>(BinaryOutput::fromResult(stmt));
  }

  /**
   * @brief Lists all binary output records
   *
   * @return Vector of deserialized `BinaryOutput` objects
   */
  std::vector<BinaryOutput> getAll() {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, deviceId, count
      FROM bo;
      )"
    );
    std::vector<BinaryOutput> vec;
    while (stmt.executeStep()) {
      vec.emplace_back(BinaryOutput::fromResult(stmt));
    }
    return vec;
  }

  /**
   * @brief Finds binary output record by deviceId
   *
   * @param deviceId Record ID
   * @return Pointer to deserialized `BinaryOutput` object, or `nullptr` if record does not exist
   */
  std::unique_ptr<BinaryOutput> getByDeviceId(const uint32_t deviceId) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, deviceId, count
      FROM bo
      WHERE deviceId = ?
      LIMIT 1;
      )"
    );
    stmt.bind(1, deviceId);
    if (!stmt.executeStep()) {
      return nullptr;
    }
    return std::make_unique<BinaryOutput>(BinaryOutput::fromResult(stmt));
  }

  /**
   * @brief Inserts new binary output record into database
   *
   * @param binaryOutput BinaryOutput object
   * @return ID of inserted record
   *
   * @throws `std::runtime_error` If the record cannot be inserted
   */
  uint32_t insert(BinaryOutput &binaryOutput) {
    SQLite::Statement stmt(*m_db,
      R"(
      INSERT INTO bo (deviceId, count)
      VALUES (?, ?);
      )"
    );
    stmt.bind(1, binaryOutput.getDeviceId());
    stmt.bind(2, binaryOutput.getCount());
    try {
      stmt.exec();
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to insert new BinaryOutput entity",
          e.what()
        )
      );
    }
    return m_db->getLastInsertRowid();
  }

  /**
   * @brief Updates existing binary output record
   *
   * @param binaryOutput BinaryOutput object
   *
   * @throws `std::runtime_error` If the record cannot be updated
   */
  void update(BinaryOutput &binaryOutput) {
    SQLite::Statement stmt(*m_db,
      R"(
      UPDATE bo
      SET deviceId = ?, count = ?
      WHERE id = ?;
      )"
    );
    stmt.bind(1, binaryOutput.getDeviceId());
    stmt.bind(2, binaryOutput.getCount());
    stmt.bind(3, binaryOutput.getId());
    try {
      stmt.exec();
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to update BinaryOutput entity ID " + std::to_string(binaryOutput.getId()),
          e.what()
        )
      );
    }
  }

  /**
   * @brief Removes existing record by ID
   *
   * @param id Record ID
   */
  void remove(const uint32_t id) {
    SQLite::Statement stmt(*m_db,
      R"(
      DELETE FROM bo
      WHERE id = ?;
      )"
    );
    stmt.bind(1, id);
    stmt.exec();
  }

  /**
   * @brief Removes existing binary output record by device ID
   *
   * @param deviceId Device ID
   */
  void removeByDeviceId(const uint32_t deviceId) {
    SQLite::Statement stmt(*m_db,
      R"(
      DELETE FROM bo
      WHERE deviceId = ?;
      )"
    );
    stmt.bind(1, deviceId);
    stmt.exec();
  }

  /**
   * @brief Finds and returns unique addresses of devices implementing binary output standard
   *
   * @return Set of device addresses
   */
  std::set<uint8_t> getAddresses() {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT d.address
      FROM bo as b
      INNER JOIN device as d ON d.id = b.deviceId;
      )"
    );
    std::set<uint8_t> addrs;
    while(stmt.executeStep()) {
      addrs.insert(static_cast<uint8_t>(stmt.getColumn(0).getUInt()));
    }
    return addrs;
  }

  /**
   * @brief Returns a map of device addresses and number of binary outputs implemented by each device
   *
   * @return Map of device addresses and binary outputs count
   */
  std::map<uint8_t, uint8_t> getAddressCountMap() {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT d.address, b.count
      FROM bo as b
      INNER JOIN device as d ON d.id = b.deviceId;
      )"
    );
    std::map<uint8_t, uint8_t> map;
    while(stmt.executeStep()) {
      map.insert(
        std::make_pair(
          static_cast<uint8_t>(stmt.getColumn(0).getUInt()),
          static_cast<uint8_t>(stmt.getColumn(1).getUInt())
        )
      );
    }
    return map;
  }
};

}
