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

#include "rapidjson/document.h"
#include "Entities.h"
#include "IqrfCommon.h"
#include "JsDriverSensor.h"

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <vector>

typedef std::tuple<iqrf::db::Device, uint16_t, uint16_t, uint16_t, std::string, uint16_t> DeviceTuple;
typedef std::tuple<uint8_t, uint8_t> AddrIndex;
typedef std::unordered_map<uint8_t, std::vector<AddrIndex>> SensorDataSelectMap;

namespace iqrf {

	/**
	 * IQRF DB interface
	 */
	class IIqrfDb {
	public:

		/**
		 * Destructor
		 */
		virtual ~IIqrfDb() {}

		/**
		 * Resets network devices database
		 */
		virtual void resetDatabase() = 0;

		///// Transactions

		virtual void beginTransaction() = 0;

		virtual void finishTransaction() = 0;

		virtual void cancelTransaction() = 0;

		///// Binary outputs

		virtual std::unique_ptr<iqrf::db::BinaryOutput> getBinaryOutput(const uint32_t &id) = 0;

		virtual std::unique_ptr<iqrf::db::BinaryOutput> getBinaryOutputByDevice(const uint32_t &deviceId) = 0;

		virtual uint32_t insertBinaryOutput(iqrf::db::BinaryOutput &binaryOutput) = 0;

		virtual void updateBinaryOutput(iqrf::db::BinaryOutput &binaryOutput) = 0;

		virtual void removeBinaryOutput(const uint32_t &deviceId) = 0;

		virtual std::map<uint8_t, uint8_t> getBinaryOutputCountMap() = 0;

		///// DALIs

		virtual std::unique_ptr<iqrf::db::Dali> getDaliByDevice(const uint32_t &deviceId) = 0;

		virtual uint32_t insertDali(iqrf::db::Dali &dali) = 0;

		virtual void removeDali(const uint32_t &deviceId) = 0;

		virtual std::set<uint8_t> getDaliDeviceAddresses() = 0;

		///// Devices

		virtual std::vector<iqrf::db::Device> getAllDevices() = 0;

		virtual std::unique_ptr<iqrf::db::Device> getDevice(const uint32_t &id) = 0;

		virtual std::unique_ptr<iqrf::db::Device> getDevice(const uint8_t &address) = 0;

		virtual uint32_t insertDevice(iqrf::db::Device &device) = 0;

		virtual void updateDevice(iqrf::db::Device &device) = 0;

		virtual void removeDevice(const uint32_t &id) = 0;

		virtual bool deviceImplementsPeripheral(const uint32_t &id, int16_t peripheral) = 0;

		virtual uint16_t getDeviceHwpid(const uint8_t &address) = 0;

		virtual uint32_t getDeviceMid(const uint8_t &address) = 0;

		virtual std::string getDeviceMetadata(const uint8_t &address) = 0;

		virtual rapidjson::Document getDeviceMetadataDoc(const uint8_t &address) = 0;

		virtual void setDeviceMetadata(const uint8_t &address, const std::string &metadata) = 0;

		virtual std::vector<DeviceTuple> getDevicesWithProductInfo(std::vector<uint8_t> requestedDevices = {}) = 0;

		///// Device sensors

		virtual std::unique_ptr<iqrf::db::DeviceSensor> getDeviceSensor(const uint32_t &id) = 0;

		virtual std::unique_ptr<iqrf::db::DeviceSensor> getDeviceSensor(const uint8_t &address, const uint32_t &sensorId, const uint8_t &index) = 0;

		virtual std::unique_ptr<iqrf::db::DeviceSensor> getDeviceSensorByGlobalIndex(const uint8_t &address, const uint8_t &index) = 0;

		virtual std::unique_ptr<iqrf::db::DeviceSensor> getDeviceSensorByTypeIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) = 0;

		virtual void insertDeviceSensor(iqrf::db::DeviceSensor &deviceSensor) = 0;

		virtual void updateDeviceSensor(iqrf::db::DeviceSensor &deviceSensor) = 0;

		virtual void removeDeviceSensors(const uint8_t &address) = 0;

		virtual std::map<uint8_t, std::vector<std::tuple<iqrf::db::DeviceSensor, iqrf::db::Sensor>>> getDeviceSensorMap() = 0;

		virtual void setDeviceSensorMetadata(iqrf::db::DeviceSensor &deviceSensor, nlohmann::json &metadata, std::shared_ptr<std::string> updated) = 0;

		virtual void setDeviceSensorMetadata(const uint8_t &address, const uint8_t &index, nlohmann::json &metadata, std::shared_ptr<std::string> updated) = 0;

		// FRC variant
		virtual void setDeviceSensorMetadata(const uint8_t &address, const uint8_t &type, const uint8_t &index, nlohmann::json &metadata, std::shared_ptr<std::string> updated) = 0;

		virtual void setDeviceSensorValue(iqrf::db::DeviceSensor &deviceSensor, double &value, std::shared_ptr<std::string> timestamp) = 0;

		virtual void setDeviceSensorValue(const uint8_t &address, const uint8_t &index, double &value, std::shared_ptr<std::string> timestamp) = 0;

		// FRC variant
		virtual void setDeviceSensorValue(const uint8_t &address, const uint8_t &type, const uint8_t &index, double &value, std::shared_ptr<std::string> timestamp) = 0;

		virtual void updateDeviceSensorValues(const std::map<uint8_t, std::vector<sensor::item::Sensor>> &devices) = 0;

		virtual void updateDeviceSensorValues(const uint8_t &address, const std::string &sensors) = 0;

		// FRC variant
		virtual void updateDeviceSensorValues(const uint8_t &type, const uint8_t &index, const std::set<uint8_t> &selectedNodes, const std::string &sensors) = 0;

		virtual uint8_t getDeviceSensorGlobalIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) = 0;

		///// Drivers

		virtual std::unique_ptr<iqrf::db::Driver> getDriver(const int16_t &peripheral, const double &version) = 0;

		virtual std::vector<iqrf::db::Driver> getDriversByProduct(const uint32_t &productId) = 0;

		virtual std::vector<uint32_t> getDriverIdsByProduct(const uint32_t &productId) = 0;

		virtual std::vector<iqrf::db::Driver> getLatestDrivers() = 0;

		virtual uint32_t insertDriver(iqrf::db::Driver &driver) = 0;

		///// Lights

		virtual std::unique_ptr<iqrf::db::Light> getLight(const uint32_t &id) = 0;

		virtual std::unique_ptr<iqrf::db::Light> getLightByDevice(const uint32_t &deviceId) = 0;

		virtual uint32_t insertLight(iqrf::db::Light &light) = 0;

		virtual void updateLight(iqrf::db::Light &light) = 0;

		virtual void removeLight(const uint32_t &deviceId) = 0;

		virtual std::map<uint8_t, uint8_t> getLightCountMap() = 0;

		///// Products

		virtual std::unique_ptr<iqrf::db::Product> getProduct(const uint32_t &id) = 0;

		virtual std::unique_ptr<iqrf::db::Product> getProduct(const uint16_t &hwpid, const uint16_t &hwpidVer, const uint16_t &osBuild, const uint16_t &dpa) = 0;

		virtual uint32_t insertProduct(iqrf::db::Product &product) = 0;

		virtual uint32_t getCoordinatorProductId() = 0;

		virtual std::vector<uint8_t> getProductDeviceAddresses(const uint32_t &productId) = 0;

		virtual std::string getProductCustomDriver(const uint32_t &productId) = 0;

		///// Product drivers

		virtual void insertProductDriver(iqrf::db::ProductDriver &productDriver) = 0;

		virtual std::map<uint32_t, std::set<uint32_t>> getProductsDriversIdMap() = 0;

		///// Sensors

		virtual std::unique_ptr<iqrf::db::Sensor> getSensor(const uint8_t &type, const std::string &name) = 0;

		virtual uint32_t insertSensor(iqrf::db::Sensor &sensor) = 0;

		virtual std::map<uint8_t, iqrf::db::Sensor> getSensorsImplementedByDeviceMap(const uint8_t &address) = 0;


		///// Other

		virtual std::map<uint16_t, std::set<uint8_t>> getHwpidAddrsMapImplementingSensor(const uint8_t &type) = 0;

		virtual SensorDataSelectMap getSensorDataSelectMap() = 0;

		/**
		 * Checks if metadata should be added to messages
		 * @return true if metadata should be added to messages, false otherwise
		 */
		virtual bool addMetadataToMessage() = 0;
	};
}
