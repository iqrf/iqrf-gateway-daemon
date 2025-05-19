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

