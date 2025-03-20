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

#include <nlohmann/json.hpp>
#include "Storage.h"
#include "EmbedNode.h"

#include <tuple>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;
using namespace sqlite_orm;

typedef std::tuple<Device, Product> DeviceProductTuple;
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
	 * Returns device records with product information from database, if vector of request devices is empty, all devices are returned
	 * @param requestedDevices Addresses of request devices
	 * @return Vector of devices with product information
	 */
	std::vector<DeviceProductTuple> getDevices(std::vector<uint8_t> requestedDevices = {});

	/**
	 * Returns addresses of devices in network from database
	 * @return std::set<uint8_t> Set of device addresses
	 */
	std::set<uint8_t> getDeviceAddrs();

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
	Device getDevice(const uint8_t &address);

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
	std::shared_ptr<std::string> getDeviceMetadata(const uint8_t &address);

	/**
	 * Sets metadata to device at specified address
	 * @param address Device address
	 * @param metadata Metadata string
	 */
	void setDeviceMetadata(const uint8_t &address, std::shared_ptr<std::string> metadata);

	/**
	 * Return map of device addresses and product IDs
	 * @return std::map<uint8_t, uint32_t> Map of device addresses and product IDs
	 */
	std::map<uint8_t, uint32_t> getDeviceProductIdMap();

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

	uint32_t getProductIdNoncertified(const uint16_t &hwpid, const uint16_t &hwpidVersion, const uint16_t &osBuild, const uint16_t &dpaVersion);

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
	 * Checks if device implements sensors
	 * @param deviceAddress Device address
	 * @return true if device implements sensors, false otherwise
	 */
	bool hasSensors(const uint8_t &deviceAddress);

	/**
	 * Returns map of device sensor indexes and sensor entities
	 * @param deviceAddress Device address
	 * @return Map of device sensor indexes and sensor entities
	 */
	std::map<uint8_t, Sensor> getDeviceSensorsByAddress(const uint8_t &deviceAddress);

	/**
	 * Returns map of device sensor indexes and sensor IDs
	 * @param deviceAddress Device address
	 * @return Map of device sensor indexes and sensor IDs
	 */
	std::map<uint8_t, uint32_t> getDeviceSensorIndexIdMap(const uint8_t &deviceAddress);

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
	 * Remove device sensor record by device address and sensor index
	 * @param address Device address
	 * @param index Sensor index
	 */
	void removeDeviceSensor(const uint8_t &address, const uint8_t &index);

	/**
	 * Remove device sensor records by device address and sensor indexes
	 * @param address Device address
	 * @param indexes Sensor indexes
	 */
	void removeDeviceSensors(const uint8_t &address, const std::vector<uint8_t> &indexes);

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
	 * @return Sensor device entity
	 */
	DeviceSensor getSensorByTypeIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index);

	/**
	 * Returns device sensor entity if it exists
	 * @param address Device address
	 * @param index Sensor index
	 * @return Sensor device entity
	 */
	DeviceSensor getDeviceSensorByIndex(const uint8_t &address, const uint8_t &index);

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
