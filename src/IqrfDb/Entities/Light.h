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
	 */
	Light(const uint32_t &deviceId) : deviceId(deviceId) {};

	/**
	 * Returns Light ID
	 * @return Light ID
	 */
	const uint32_t& getId() const;

	/**
	 * Sets Light ID
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
private:
	/// Light ID
	uint32_t id;
	/// Device ID
	uint32_t deviceId;
};
