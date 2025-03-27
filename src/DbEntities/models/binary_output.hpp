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
 * IQRF DB BinaryOutput entity
 */
class BinaryOutput {
public:
	/**
	 * Base constructor
	 */
	BinaryOutput() = default;

	/**
	 * Constructor without ID
	 * @param deviceId Device ID
	 * @param count Implemented binary outputs
	 */
	BinaryOutput(const uint32_t deviceId, const uint8_t count) : deviceId(deviceId), count(count) {};

	/**
	 * Full constructor
	 * @param id ID
	 * @param deviceId Device ID
	 * @param count Implemented binary outputs
	 */
	BinaryOutput(const uint32_t id, const uint32_t deviceId, const uint8_t count)
		: id(id),
		  deviceId(deviceId),
		  count(count) {};

	/**
	 * Returns device ID
	 * @return Device ID
	 */
	uint32_t getId() const {
		return id;
	}

	/**
	 * Sets device ID
	 * @param id Device ID
	 */
	void setId(const uint32_t id) {
		this->id = id;
	}

	/**
	 * Returns device ID
	 * @return Device ID
	 */
	uint32_t getDeviceId() const {
		return deviceId;
	}

	/**
	 * Sets device ID
	 * @param deviceId Device ID
	 */
	void setDeviceId(const uint32_t deviceId) {
		this->deviceId = deviceId;
	}

	/**
	 * Returns number of implemented binary outputs
	 * @return Binary outputs count
	 */
	uint8_t getCount() const {
		return count;
	}

	/**
	 * Sets number of implemented binary outputs
	 * @param count Binary outputs count
	 */
	void setCount(const uint8_t count) {
		this->count = count;
	}

	static BinaryOutput fromResult(SQLite::Statement &stmt) {
		auto id = stmt.getColumn(0).getUInt();
		auto deviceId = stmt.getColumn(1).getUInt();
		auto count = static_cast<uint8_t>(stmt.getColumn(2).getUInt());
		return BinaryOutput(id, deviceId, count);
	}
private:
	/// Binary output ID
	uint32_t id;
	/// Device ID
	uint32_t deviceId;
	/// Implemented binary outputs
	uint8_t count;
};

}

