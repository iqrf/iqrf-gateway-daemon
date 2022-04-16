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
 * IQRF DB Light entity
 */
class Light {
public:
	/**
	 * Base constructor
	 */
	Light() = default;

	/**
	 * Full constructor
	 * @param deviceId Device ID
	 * @param count Implemented lights
	 */
	Light(const uint32_t &deviceId, const uint8_t &count) : deviceId(deviceId), count(count) {};

	/**
	 * Returns light ID
	 * @return Light ID
	 */
	const uint32_t& getId() const;

	/**
	 * Sets light ID
	 * @param id Light ID
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

	/**
	 * Returns number of implemented lights
	 * @return Light count
	 */
	const uint8_t& getCount() const;

	/**
	 * Sets number of implemented lights
	 * @param count Light count
	 */
	void setCount(const uint8_t &count);
private:
	/// Light ID
	uint32_t id;
	/// Device ID
	uint32_t deviceId;
	/// Implemented lights
	uint8_t count;
};
