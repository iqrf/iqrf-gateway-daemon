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
#include <vector>

#include <models/driver.hpp>
#include <models/product_driver.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::ProductDriver;

namespace iqrf::db::repos {

/**
 * Product driver repository
 */
class ProductDriverRepository : public BaseRepository {
public:
  using BaseRepository::BaseRepository;

  /**
   * @brief Lists all product driver records
   *
   * @return Vector of deserialized `ProductDriver` objects
   */
  std::vector<ProductDriver> getAll() {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT productId, driverId
      FROM productDriver;
      )"
    );
    std::vector<ProductDriver> vector;
    while (stmt.executeStep()) {
      vector.emplace_back(ProductDriver::fromResult(stmt));
    }
    return vector;
  }

  /**
   * @brief Inserts new product driver record into database
   *
   * @param productDriver Product driver object
   * @return ID of inserted record
   *
   * @throws `std::runtime_error` If the record cannot be inserted
   */
  uint32_t insert(ProductDriver &productDriver) {
    SQLite::Statement stmt(*m_db,
      R"(
      INSERT INTO productDriver (productId, driverId)
      VALUES (?, ?);
      )"
    );
    stmt.bind(1, productDriver.getProductId());
    stmt.bind(2, productDriver.getDriverId());
    try {
      stmt.exec();
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to insert new ProductDriver entity",
          e.what()
        )
      );
    }
    return m_db->getLastInsertRowid();
  }

  /**
   * @brief Removes existing product driver record by product and driver IDs
   *
   * @param productId Product ID
   * @param driverId Driver ID
   */
  void remove(const uint32_t productId, const uint32_t driverId) {
    SQLite::Statement stmt(*m_db,
      R"(
      DELETE FROM productDriver
      WHERE productId = ? AND driverId = ?;
      )"
    );
    stmt.bind(1, productId);
    stmt.bind(2, driverId);
    stmt.exec();
  }

  /**
   * @brief Finds and returns all product drivers
   *
   * @param productId Product ID
   * @return Vector of deserialized `Driver` objects
   */
  std::vector<Driver> getDrivers(const uint32_t productId) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT d.id, d.name, d.peripheralNumber, d.version, d.versionFlags, d.driver, d.driverHash
      FROM driver as d
      INNER JOIN productDriver as pd ON pd.driverId = d.id
      WHERE pd.productId = ?;
      )"
    );
    stmt.bind(1, productId);
    std::vector<Driver> vector;
    while (stmt.executeStep()) {
      vector.emplace_back(Driver::fromResult(stmt));
    }
    return vector;
  }

  /**
   * @brief Finds and returns IDs of drivers records for product
   *
   * @param productId Product ID
   *
   * @return Set of driver IDs
   */
  std::set<uint32_t> getDriverIds(const uint32_t productId) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT driverId
      FROM productDriver
      WHERE productId = ?;
      )"
    );
    stmt.bind(1, productId);
    std::set<uint32_t> set;
    while (stmt.executeStep()) {
      set.insert(stmt.getColumn(0).getUInt());
    }
    return set;
  }

  /**
   * @brief Constructs and returns map of product and driver IDs
   *
   * @return Map of product IDs and sets of driver IDs
   */
  std::map<uint32_t, std::set<uint32_t>> getProductsDriversMap() {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT p.id, d.id
      FROM driver as d
      INNER JOIN productDriver as pd ON pd.driverId = d.id
      INNER JOIN product as p ON p.id = pd.productId;
      )"
    );
    std::map<uint32_t, std::set<uint32_t>> map;
    while (stmt.executeStep()) {
      auto productId = stmt.getColumn(0).getUInt();
      auto driverId = stmt.getColumn(0).getUInt();
      if (map.count(productId) == 0) {
        map.insert(
          std::make_pair(
            productId,
            std::set<uint32_t>{driverId}
          )
        );
      } else {
        map[productId].insert(driverId);
      }
    }
    return map;
  }
};

}
