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

#include <nlohmann/json.hpp>
#include "Storage.h"
#include "EmbedNode.h"

#include <tuple>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;
using namespace sqlite_orm;

typedef std::tuple<Device, uint16_t, uint16_t, uint16_t, std::string, uint16_t> DeviceTuple;
typedef std::tuple<uint8_t, uint8_t> AddrIndex;
typedef std::unordered_map<uint8_t, std::vector<AddrIndex>> SensorSelectMap;

class QueryHandler {
public:
	/**
	 * Base constructor
	 */
	QueryHandler() = default;

	/**
	 * Constructor
	 * @param db Database pointer
	 */
	QueryHandler(std::shared_ptr<Storage> &db);

	/**
	 * Returns all device records with product information from database
	 * @return Vector of devices with product information
	 */
	std::vector<DeviceTuple> getDevices();

	/**
	 * Checks if device record exists
	 * @param address Device address
	 * @return true if device record exists, false otherwise
	 */
	bool deviceExists(const uint8_t &address);

	/**
	 * Returns device at specified address
	 * @param address Device address
	 * @return Device
	 */
	std::vector<Device> getDevice(const uint8_t &address);

	/**
	 * Returns device MID
	 * @param addresses Device address
	 * @return Device MID
	 */
	uint32_t getDeviceMid(const uint8_t &address);

	/**
	 * Returns device HWPID
	 * @param address Device address
	 * @return Device HWPID
	 */
	uint16_t getDeviceHwpid(const uint8_t &address);

	/**
	 * Returns metadata stored for device at specified address
	 * @param address Device address
	 * @return Device metadata string
	 */
	std::string getDeviceMetadata(const uint8_t &address);

	/**
	 * Sets metadata to device at specified address
	 * @param address Device address
	 * @param metadata Metadata string
	 */
	void setDeviceMetadata(const uint8_t &address, const std::string &metadata);

	/**
	 * Returns ID of coordinator device
	 * @param db Database pointer
	 * @return ID of coordinator device, 0 if C device doesn't exist
	 */
	std::uint32_t getCoordinatorProductId();

	/**
	 * Checks if product record exists in database
	 * @param hwpid HWPID
	 * @param hwpidVersion HWPID version
	 * @param osBuild OS build
	 * @param dpaVersion DPA version
	 * @return Product record ID, 0 if no such record exists
	 */
	uint32_t getProductId(const uint16_t &hwpid, const uint16_t &hwpidVersion, const uint16_t &osBuild, const uint16_t &dpaVersion);

	Product getProductById(const uint32_t &productId);

	/**
	 * Returns addresses of devices that match product ID
	 * @param productID Product ID
	 * @return Vector of device addresses that match the product ID
	 */
	std::vector<uint8_t> getProductAddresses(const uint32_t &productId);

	/**
	 * Returns product IDs and their corresponding driver IDs
	 * @return Map of product IDs and their driver ID sets
	 */
	std::map<uint32_t, std::set<uint32_t>> getProductsDriversMap();

	/**
	 * Returns product drivers
	 * @param productID Product ID
	 * @return Vector of product drivers
	 */
	std::vector<Driver> getProductDrivers(const uint32_t &productId);

	/**
	 * Returns product drivers IDs
	 * @param productID Product ID
	 * @return Vector of product drivers IDs
	 */
	std::set<uint32_t> getProductDriversIds(const uint32_t &productId);

	/**
	 * Returns product custom driver
	 * @return Product custom driver
	 */
	std::string getProductCustomDriver(const uint32_t &productId);

	/**
	 * Returns vector for newest drivers for each peripheral
	 * @return Vector of newest drivers per peripheral
	 */
	std::vector<Driver> getNewestDrivers();

	/**
	 * Checks if a device implements BinaryOutput standard
	 * @param deviceId Device ID
	 * @return true if device implements BinaryOutput standard, false otherwise
	 */
	bool boExists(const uint32_t &deviceId);

	/**
	 * Returns ID of BinaryOutput record
	 * @param deviceId Device ID
	 * @return BinaryOutput record ID, 0 if no such record exists
	 */
	uint32_t getBoId(const uint32_t &deviceId);

	/**
	 * Returns all BinaryOutput device records from database
	 * @return Map of device addresses and implemented binary outputs
	 */
	std::map<uint8_t, uint8_t> getBinaryOutputs();

	/**
	 * Remove all BinaryOutput records implemented by device ID
	 * @param deviceId Device ID
	 */
	void removeBinaryOutputs(const uint32_t &deviceId);

	/**
	 * Checks if a device implements DALI standard
	 * @param deviceId Device ID
	 * @return true if device implements DALI standard, false otherwise
	 */
	bool daliExists(const uint32_t &deviceId);

	/**
	 * Returns all DALI device records from database
	 * @return Set of device addresses implementing DALI
	 */
	std::set<uint8_t> getDalis();

	/**
	 * Remove all DALI records implemented by device ID
	 * @param deviceId Device ID
	 */
	void removeDalis(const uint32_t &deviceId);

	/**
	 * Checks if a device implements light standard
	 * @param deviceId Device ID
	 * @return true if device implements Light, false otherwise
	 */
	bool lightExists(const uint32_t &deviceId);

	/**
	 * Returns ID of Light record
	 * @param deviceId Device ID
	 * @return Light record ID, 0 if no such record exists
	 */
	uint32_t getLightId(const uint32_t &deviceId);

	/**
	 * Returns all Light device records from database
	 * @return Map of device addresses and implemented lights
	 */
	std::map<uint8_t, uint8_t> getLights();

	/**
	 * Remove all Light records implemented by device ID
	 * @param deviceId Device ID
	 */
	void removeLights(const uint32_t &deviceId);

	/**
	 * Checks if device implements sensors
	 * @param deviceAddress Device address
	 * @return true if device implements sensors, false otherwise
	 */
	bool hasSensors(const uint8_t &deviceAddress);

	std::map<uint8_t, Sensor> getDeviceSensorsByAddress(const uint8_t &deviceAddress);

	/**
	 * Checks if a sensor type exists in database
	 * @param type Sensor type
	 * @param name Sensor name
	 * @return true if sensor type is in database, false otherwise
	 */
	bool sensorTypeExists(const uint8_t &type, const std::string &name);

	/**
	 * Returns ID of sensor record specified by type
	 * @param type Sensor type
	 * @param name Sensor name
	 * @return Sensor record ID, 0 if no such record exists
	 */
	uint32_t getSensorId(const uint8_t &type, const std::string &name);

	/**
	 * Checks if device implementing sensor record is in database
	 * @param address Device address
	 * @param sensorId Sensor ID
	 * @param index Sensor index
	 * @return true if record exists, false otherwise
	 */
	bool deviceSensorExists(const uint8_t &address, const uint32_t &sensorId, const uint8_t &index);

	/**
	 * Returns all sensor device records from database
	 * @return Map of device addresses and implemented sensors
	 */
	std::map<uint8_t, std::vector<std::tuple<DeviceSensor, Sensor>>> getSensors();

	/**
	 * Constructs and returns a map of sensor types, devices that implement them and their local indexes
	 * @return Map of sensor types and devices
	 */
	SensorSelectMap constructSensorSelectMap();

	/**
	 * Retrieves global sensor index from address, type and type index
	 * @param address Device address
	 * @param type Sensor type
	 * @param index Type index
	 * @return Global sensor index
	 */
	uint8_t getGlobalSensorIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index);

	/**
	 * Remove all device Sensor records implemented by device ID
	 * @param address Device address
	 */
	void removeSensors(const uint8_t &address);

	/**
	 * Stores value of sensor
	 * @param address Device address
	 * @param type Sensor type
	 * @param index Sensor index
	 * @param value Last measured value
	 * @param updated Last updated
	 * @param frc Is frc?
	 */
	void setSensorValue(const uint8_t &address, const uint8_t &type, const uint8_t &index, const double &value, std::shared_ptr<std::string> updated, bool frc = false);

	/**
	 * Stores sensor metadata
	 * @param address Device address
	 * @param type Sensor type
	 * @param index Sensor index
	 * @param metadata Sensor metadata
	 * @param updated Last updated
	 * @param frc Is FRC?
	 */
	void setSensorMetadata(const uint8_t &address, const uint8_t &type, const uint8_t &index, json &metadata, std::shared_ptr<std::string> updated, bool frc = false);

	/**
	 * Returns device sensor entity if it exists
	 * @param address Device address
	 * @param type Sensor type
	 * @param index Sensor index
	 * @return Sensor device object
	 */
	DeviceSensor getSensorByTypeIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index);

	/**
	 * Finds and returns all sensors of a type implemented by a device
	 * @param address Device address
	 * @param type Sensor type
	 * @return std::vector<DeviceSensor>
	 */
	std::vector<DeviceSensor> getSensorsOfType(const uint8_t &address, const uint8_t &type);

	/**
	 * Returns unique address and hwpid of devices implementing sensor device specified by type and index
	 * @param type Sensor type
	 * @return Map of hwpids and set of device addresses
	 */
	std::map<uint16_t, std::set<uint8_t>> getSensorDeviceHwpids(const uint8_t &type);

	std::map<uint8_t, iqrf::embed::node::NodeMidHwpid> getNodeMidHwpidMap();
private:
	/// Database
	std::shared_ptr<Storage> db;
};
