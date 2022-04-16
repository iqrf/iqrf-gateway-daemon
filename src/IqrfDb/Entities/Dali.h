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
 * IQRF DB DALI entity
 */
class Dali {
public:
	/**
	 * Base constructor
	 */
	Dali() = default;

	/**
	 * Full constructor
	 * @param deviceId Device ID
	 */
	Dali(const uint32_t &deviceId) : deviceId(deviceId) {};

	/**
	 * Returns DALI ID
	 * @return DALI ID
	 */
	const uint32_t& getId() const;

	/**
	 * Sets DALI ID
	 * @param id DALI ID
	 */
	void setId(const uint32_t &id);

	/**
	 * Returns device ID
	 * @return Device ID
	 */
	const uint32_t& getDeviceId() const;

	/**
	 * Sets device ID
	 * @param deviceId Device ID
	 */
	void setDeviceId(const uint32_t &deviceId);
private:
	/// DALI ID
	uint32_t id;
	/// Device ID
	uint32_t deviceId;
};
