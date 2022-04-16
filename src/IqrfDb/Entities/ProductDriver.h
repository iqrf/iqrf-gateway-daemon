/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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
	ProductDriver(const uint32_t &productId, const uint32_t &driverId) : productId(productId), driverId(driverId) {};

	/**
	 * Returns product ID
	 * @return Product ID
	 */
	const uint32_t& getProductId() const;

	/**
	 * Sets product ID
	 * @param productId Product ID
	 */
	void setProductId(const uint32_t &productId);

	/**
	 * Returns driver ID
	 * @return Driver ID
	 */
	const uint32_t& getDriverId() const;

	/**
	 * Sets driver ID
	 * @param driverId Driver ID
	 */
	void setDriverId(const uint32_t &driverId);
private:
	/// Product ID
	uint32_t productId;
	/// Driver ID
	uint32_t driverId;
};
