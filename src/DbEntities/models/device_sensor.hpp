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
#include <memory>
#include <optional>

#include <SQLiteCpp/SQLiteCpp.h>

namespace iqrf::db::models {

/**
 * IQRF DB device sensor entity
 */
class DeviceSensor {
public:
	/**
	 * Base constructor
	 */
	DeviceSensor() = default;

	/**
	 * Full constructor
	 * @param address Device address
	 * @param type Sensor type
	 * @param globalIndex Global index
	 * @param typeIndex TypeIndex
	 * @param sensorId Sensor ID
	 * @param value Last sensor value
	 */
	DeviceSensor(const uint8_t address, const uint8_t type, const uint8_t globalIndex, const uint8_t typeIndex,
		const uint32_t sensorId, std::optional<double> value = std::nullopt,
		std::shared_ptr<std::string> updated = nullptr, std::shared_ptr<std::string> metadata = nullptr)
		: address(address),
		  type(type),
		  globalIndex(globalIndex),
		  typeIndex(typeIndex),
		  sensorId(sensorId),
		  value(value),
		  updated(updated),
		  metadata(metadata) {}

	/**
	 * Returns device ID
	 * @return Device ID
	 */
	uint8_t getAddress() const {
		return address;
	}

	/**
	 * Sets device ID
	 * @param deviceId Device ID
	 */
	void setAddress(const uint8_t &address) {
		this->address = address;
	}

	/**
	 * Returns Sensor type
	 * @return Sensor type
	 */
	uint8_t getType() const {
		return type;
	}

	/**
	 * Sets Sensor type
	 * @param type Sensor Type
	 */
	void setType(const uint8_t type) {
		this->type = type;
	}

	/**
	 * Returns global sensor index
	 * @return Global sensor index
	 */
	uint8_t getGlobalIndex() const {
		return globalIndex;
	}

	/**
	 * Sets global sensor index
	 * @param globalIndex Global sensor index
	 */
	void setGlobalIndex(const uint8_t globalIndex) {
		this->globalIndex = globalIndex;
	}

	/**
	 * Returns type sensor index
	 * @return Type sensor index
	 */
	uint8_t getTypeIndex() const {
		return typeIndex;
	}

	/**
	 * Sets type sensor index
	 * @param globalIndex Type sensor index
	 */
	void setTypeIndex(const uint8_t typeIndex) {
		this->typeIndex = typeIndex;
	}

	/**
	 * Returns Sensor ID
	 * @return Sensor ID
	 */
	uint32_t getSensorId() const {
		return sensorId;
	}

	/**
	 * Sets Sensor ID
	 * @param sensorId Sensor ID
	 */
	void setSensorId(const uint32_t sensorId) {
		this->sensorId = sensorId;
	}

	/**
	 * Returns last Sensor value
	 * @return Last Sensor value
	 */
	std::optional<double> getValue() const {
		return value;
	}

	/**
	 * Sets last Sensor value
	 * @param value Last Sensor value
	 */
	void setValue(std::optional<double> value = std::nullopt) {
		this->value = value;
	}

	/**
	 * Returns last updated timestamp
	 * @return Last updated timestamp
	 */
	std::shared_ptr<std::string> getUpdated() const {
		return updated;
	}

	/**
	 * Sets last updated timestamp
	 * @param updated Last updated timestamp
	 */
	void setUpdated(std::shared_ptr<std::string> updated = nullptr) {
		this->updated = updated;
	}

	/**
	 * Returns sensor metadata
	 * @return Sensor metadata
	 */
	std::shared_ptr<std::string> getMetadata() const {
		return metadata;
	}

	/**
	 * Sets sensor metadata
	 * @param metadata Sensor metadata
	 */
	void setMetadata(std::shared_ptr<std::string> metadata = nullptr) {
		this->metadata = metadata;
	}

	static DeviceSensor fromResult(SQLite::Statement &stmt) {
		auto address = static_cast<uint8_t>(stmt.getColumn(0).getUInt());
		auto type = static_cast<uint8_t>(stmt.getColumn(1).getUInt());
		auto globalIndex = static_cast<uint8_t>(stmt.getColumn(2).getUInt());
		auto typeIndex = static_cast<uint8_t>(stmt.getColumn(3).getUInt());
		auto sensorId = stmt.getColumn(4).getUInt();
		std::optional<double> value = std::nullopt;
		if (!stmt.getColumn(5).isNull()) {
			value = stmt.getColumn(5).getDouble();	
		}
		std::shared_ptr<std::string> updated = nullptr;
		if (!stmt.getColumn(6).isNull()) {
			updated = std::make_shared<std::string>(stmt.getColumn(6).getString());
		}
		std::shared_ptr<std::string> metadata = nullptr;
		if (!stmt.getColumn(7).isNull()) {
			metadata = std::make_shared<std::string>(stmt.getColumn(7).getString());
		}
		return DeviceSensor(address, type, globalIndex, typeIndex, sensorId, value, std::move(updated),
			std::move(metadata));
	}

private:
	/// Device address
	uint8_t address;
	/// Sensor type
	uint8_t type;
	/// Sensor index per device
	uint8_t globalIndex;
	/// Sensor index per type
	uint8_t typeIndex;
	/// Sensor ID
	uint32_t sensorId;
	/// Last recorded value
	std::optional<double> value;
	/// Last updated
	std::shared_ptr<std::string> updated;
	/// Sensor device metadata
	std::shared_ptr<std::string> metadata;
};

}

