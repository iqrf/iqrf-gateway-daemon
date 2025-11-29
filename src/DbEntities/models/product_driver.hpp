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
 * IQRF DB product driver entity
 */
class ProductDriver {
public:
  /**
   * Base constructor
   */
  ProductDriver() = default;

  /**
   * Full constructor
   * @param productId Product ID
   * @param driverId Driver ID
   */
  ProductDriver(const uint32_t productId, const uint32_t driverId) : productId(productId), driverId(driverId) {}

  /**
   * Returns product ID
   * @return Product ID
   */
  uint32_t getProductId() const {
    return productId;
  }

  /**
   * Sets product ID
   * @param productId Product ID
   */
  void setProductId(const uint32_t productId) {
    this->productId = productId;
  }

  /**
   * Returns driver ID
   * @return Driver ID
   */
  uint32_t getDriverId() const {
    return driverId;
  }

  /**
   * Sets driver ID
   * @param driverId Driver ID
   */
  void setDriverId(const uint32_t driverId) {
    this->driverId = driverId;
  }

  /**
   * @brief Creates a ProductDriver object from SQLite::Statement query result.
   *
   * @param stmt SQLiteCpp statement object
   * @return A new `ProductDriver` constructed from query result.
   */
  static ProductDriver fromResult(SQLite::Statement &stmt) {
    auto productId = stmt.getColumn(0).getUInt();
    auto driverId = stmt.getColumn(1).getUInt();
    return ProductDriver(productId, driverId);
  }
private:
  /// Product ID
  uint32_t productId;
  /// Driver ID
  uint32_t driverId;
};

}

