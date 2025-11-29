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

#include <models/product.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::Product;

namespace iqrf::db::repos {

class ProductRepository : public BaseRepository {
public:
  using BaseRepository::BaseRepository;

  /**
   * @brief Finds product record by ID
   *
   * @param id Record ID
   * @return Pointer to deserialized `Product` object, or `nullptr` if record does not exist
   */
  std::unique_ptr<Product> get(const uint32_t id) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, hwpid, hwpidVersion, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash, customDriver,
        packageId, name
      FROM product
      WHERE id = ?
      LIMIT 1;
      )"
    );
    stmt.bind(1, id);
    if (!stmt.executeStep()) {
      return nullptr;
    }
    return std::make_unique<Product>(Product::fromResult(stmt));
  }

  /**
   * @brief Finds product by HWPID, HWPID version, OS build and DPA version
   *
   * @param hwpid HWPID
   * @param hwpidVersion HWPID version
   * @param osBuild OS build
   * @param dpaVersion DPA version
   * @return Pointer to deserialized `Product` object, or `nullptr` if record does not exist
   */
  std::unique_ptr<Product> get(const uint16_t hwpid, const uint16_t hwpidVersion, const uint16_t osBuild, const uint16_t dpaVersion) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, hwpid, hwpidVersion, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash, customDriver,
        packageId, name
      FROM product
      WHERE hwpid = ? AND hwpidVersion = ? AND osBuild = ? AND dpaVersion = ?
      LIMIT 1;
      )"
    );
    stmt.bind(1, hwpid);
    stmt.bind(2, hwpidVersion);
    stmt.bind(3, osBuild);
    stmt.bind(4, dpaVersion);
    if (!stmt.executeStep()) {
      return nullptr;
    }
    return std::make_unique<Product>(Product::fromResult(stmt));
  }

  /**
   * @brief Lists all product records
   *
   * @return Vector of deserialized `Product` objects
   */
  std::vector<Product> getAll() {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, hwpid, hwpidVersion, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash, customDriver,
        packageId, name
      FROM product;
      )"
    );
    std::vector<Product> vec;
    while (stmt.executeStep()) {
      vec.emplace_back(Product::fromResult(stmt));
    }
    return vec;
  }

  /**
   * @brief Inserts new product record into database
   *
   * @param product Product object
   * @return ID of inserted record
   *
   * @throws `std::runtime_error` If the record cannot be inserted
   */
  uint32_t insert(Product &product) {
    SQLite::Statement stmt(*m_db,
      R"(
      INSERT INTO product (hwpid, hwpidVersion, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash,
        customDriver, packageId, name)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
      )"
    );
    stmt.bind(1, product.getHwpid());
    stmt.bind(2, product.getHwpidVersion());
    stmt.bind(3, product.getOsBuild());
    stmt.bind(4, product.getOsVersion());
    stmt.bind(5, product.getDpaVersion());
    if (product.getHandlerUrl() == nullptr) {
      stmt.bind(6);
    } else {
      stmt.bind(6, *product.getHandlerUrl());
    }
    if (product.getHandlerHash() == nullptr) {
      stmt.bind(7);
    } else {
      stmt.bind(7, *product.getHandlerHash());
    }
    if (product.getCustomDriver() == nullptr) {
      stmt.bind(8);
    } else {
      stmt.bind(8, *product.getCustomDriver());
    }
    if (product.getPackageId() == std::nullopt) {
      stmt.bind(9);
    } else {
      stmt.bind(9, product.getPackageId().value());
    }
    if (product.getName() == nullptr) {
      stmt.bind(10);
    } else {
      stmt.bind(10, *product.getName());
    }
    try {
      stmt.exec();
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to insert new Product entity",
          e.what()
        )
      );
    }
    return m_db->getLastInsertRowid();
  }

  /**
   * @brief Updates existing product record
   *
   * @param product Product object
   *
   * @throws `std::runtime_error` If the record cannot be updated
   */
  void update(Product &product) {
    SQLite::Statement stmt(*m_db,
      R"(
      UPDATE product
      SET hwpid = ?, hwpidVersion = ?, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash, customDriver,
        packageId, name
      WHERE id = ?;
      )"
    );
    stmt.bind(1, product.getHwpid());
    stmt.bind(2, product.getHwpidVersion());
    stmt.bind(3, product.getOsBuild());
    stmt.bind(4, product.getOsVersion());
    stmt.bind(5, product.getDpaVersion());
    if (product.getHandlerUrl() == nullptr) {
      stmt.bind(6);
    } else {
      stmt.bind(6, *product.getHandlerUrl());
    }
    if (product.getHandlerHash() == nullptr) {
      stmt.bind(7);
    } else {
      stmt.bind(7, *product.getHandlerHash());
    }
    if (product.getCustomDriver() == nullptr) {
      stmt.bind(8);
    } else {
      stmt.bind(8, *product.getCustomDriver());
    }
    if (product.getPackageId() == std::nullopt) {
      stmt.bind(9);
    } else {
      stmt.bind(9, product.getPackageId().value());
    }
    if (product.getName() == nullptr) {
      stmt.bind(10);
    } else {
      stmt.bind(10, *product.getName());
    }
    stmt.bind(11, product.getId());
    try {
      stmt.exec();
    } catch (const SQLite::Exception &e) {
      throw std::runtime_error(
        this->formatErrorMessage(
          "Failed to update product entity ID " + std::to_string(product.getId()),
          e.what()
        )
      );
    }
  }

  /**
   * @brief Finds coordinator product and returns ID
   *
   * @return Optional value container, product ID if record exists, `std::nullopt` otherwise
   */
  std::optional<uint32_t> getCoordinatorProductId() {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT p.id
      FROM product as p
      INNER JOIN device as d on d.productId = p.id
      WHERE d.address = 0
      LIMIT 1;
      )"
    );
    if (!stmt.executeStep()) {
      return std::nullopt;
    }
    return stmt.getColumn(0).getUInt();
  }

  /**
   * @brief Finds noncertified product and returns record ID
   *
   * @param hwpid HWPID
   * @param hwpidVersion HWPID version
   * @param osBuild OS build
   * @param dpaVersion DPA version
   * @return Optional value container, product ID if record exists `std::nullopt` otherwise
   */
  std::optional<uint32_t> getNoncertifiedProductId(const uint16_t hwpid, const uint16_t hwpidVersion,
    const uint16_t osBuild, const uint16_t dpaVersion) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT id, hwpid, hwpidVersion, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash, customDriver,
        packageId, name
      FROM product
      WHERE hwpid = ? AND hwpidVersion = ? AND osBuild = ? AND dpaVersion = ? and packageId = ?
      LIMIT 1;
      )"
    );
    stmt.bind(1, hwpid);
    stmt.bind(2, hwpidVersion);
    stmt.bind(3, osBuild);
    stmt.bind(4, dpaVersion);
    stmt.bind(5);
    if (!stmt.executeStep()) {
      return std::nullopt;
    }
    return stmt.getColumn(0).getUInt();
  }

  /**
   * @brief Finds product record and returns custom product driver
   *
   * @param productId Product ID
   * @return Pointer to product driver, if record exists and product has custom driver, `nullptr` otherwise
   */
  std::shared_ptr<std::string> getCustomDriver(const uint32_t productId) {
    SQLite::Statement stmt(*m_db,
      R"(
      SELECT customDriver
      FROM product
      WHERE id = ?
      LIMIT 1;
      )"
    );
    stmt.bind(1, productId);
    if (!stmt.executeStep() || stmt.getColumn(0).isNull()) {
      return nullptr;
    }
    return std::make_shared<std::string>(stmt.getColumn(0).getString());
  }
};

}
