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
#include <memory>

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
	DeviceSensor(const uint8_t &address, const uint8_t &type, const uint8_t &globalIndex, const uint8_t &typeIndex, const uint32_t &sensorId, std::shared_ptr<double> value = nullptr);

	/**
	 * Returns device ID
	 * @return Device ID
	 */
	const uint8_t& getAddress() const;

	/**
	 * Sets device ID
	 * @param deviceId Device ID
	 */
	void setAddress(const uint8_t &address);

	/**
	 * Returns Sensor type
	 * @return Sensor type
	 */
	const uint8_t& getType() const;

	/**
	 * Sets Sensor type
	 * @param type Sensor Type
	 */
	void setType(const uint8_t &type);

	/**
	 * Returns global sensor index
	 * @return Global sensor index
	 */
	const uint8_t& getGlobalIndex() const;

	/**
	 * Sets global sensor index
	 * @param globalIndex Global sensor index
	 */
	void setGlobalIndex(const uint8_t &globalIndex);

	/**
	 * Returns type sensor index
	 * @return Type sensor index
	 */
	const uint8_t& getTypeIndex() const;

	/**
	 * Sets type sensor index
	 * @param globalIndex Type sensor index
	 */
	void setTypeIndex(const uint8_t &typeIndex);

	/**
	 * Returns Sensor ID
	 * @return Sensor ID
	 */
	const uint32_t& getSensorId() const;

	/**
	 * Sets Sensor ID
	 * @return Sensor ID
	 */
	void setSensorId(const uint32_t &sensorId);

	/**
	 * Returns last Sensor value
	 * @return Last Sensor value
	 */
	std::shared_ptr<double> getValue() const;

	/**
	 * Sets last Sensor value
	 * @param value Last Sensor value
	 */
	void setValue(std::shared_ptr<double> value);

	/**
	 * Returns last updated timestamp
	 * @return Last updated timestamp
	 */
	std::shared_ptr<std::string> getUpdated() const;

	/**
	 * Sets last updated timestamp
	 * @param updated Last updated timestamp
	 */
	void setUpdated(std::shared_ptr<std::string> updated);

	/**
	 * Returns sensor metadata
	 * @return Sensor metadata
	 */
	std::shared_ptr<std::string> getMetadata() const;

	/**
	 * Sets sensor metadata
	 * @param metadata Sensor metadata
	 */
	void setMetadata(std::shared_ptr<std::string> metadata);

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
	std::shared_ptr<double> value = nullptr;
	/// Last updated
	std::shared_ptr<std::string> updated = nullptr;
	/// Sensor device metadata
	std::shared_ptr<std::string> metadata = nullptr;
};
