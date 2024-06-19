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
#include "../IqrfDb/Entities/BinaryOutput.h"
#include "../IqrfDb/Entities/Dali.h"
#include "../IqrfDb/Entities/Device.h"
#include "../IqrfDb/Entities/DeviceSensor.h"
#include "../IqrfDb/Entities/Driver.h"
#include "../IqrfDb/Entities/Light.h"
#include "../IqrfDb/Entities/Product.h"
#include "../IqrfDb/Entities/ProductDriver.h"
#include "../IqrfDb/Entities/Sensor.h"
#include "JsDriverSensor.h"

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <vector>

typedef std::tuple<Device, uint16_t, uint16_t, uint16_t, std::string, uint16_t> DeviceTuple;
typedef std::tuple<uint8_t, uint8_t> AddrIndex;
typedef std::unordered_map<uint8_t, std::vector<AddrIndex>> SensorDataSelectMap;

namespace iqrf {

	/**
	 * IQRF DB interface
	 */
	class IIqrfDb {
	public:
		/**
		 * Enumeration parameters
		 */
		struct EnumParams {
			bool reenumerate = false;
			bool standards = false;
		};

		/**
		 * Enumeration progress class
		 */
		class EnumerationProgress {
		public:
			/**
			 * Enumeration progress steps
			 */
			enum Steps {
				Start,
				NetworkDone,
				Devices,
				DevicesDone,
				Products,
				ProductsDone,
				Standards,
				StandardsDone,
				Finish
			};

			/**
			 * Base constructor
			 */
			EnumerationProgress() {}

			/**
			 * Full constructor
			 * @param step Enumeration step
			 */
			EnumerationProgress(Steps step) : step(step) {};

			/**
			 * Returns enumeration step
			 * @return Enumeration step
			 */
			Steps getStep() { return step; }

			/**
			 * Returns message corresponding to the step
			 * @param step Current step
			 * @return Step message
			 */
			std::string getStepMessage() { return stepMessages[step]; }
		private:
			/// Enumeration step
			Steps step = Steps::Start;
			/// Map of enumeration steps and messages
			std::map<Steps, std::string> stepMessages = {
				{Steps::Start, "Enumeration started, checking current state of the network."},
				{Steps::NetworkDone, "Finished checking current state of network."},
				{Steps::Devices, "Enumerating device information."},
				{Steps::DevicesDone, "Finished enumerating device information."},
				{Steps::Products, "Enumerating product information."},
				{Steps::ProductsDone, "Finished enumerating product information."},
				{Steps::Standards, "Enumerating device standards."},
				{Steps::StandardsDone, "Finished enumerating standards."},
				{Steps::Finish, "Enumeration finished."}
			};
		};

		/**
		 * Check if enumeration is in progress
		 * @return true if enumeration is in progress, false otherwise
		 */
		virtual bool isRunning() = 0;

		/**
		 * Runs enumeration
		 * @param reenumerate Executes full enumeration regardless of the database contents
		 * @param standards Enumerates standards
		 */
		virtual void enumerate(IIqrfDb::EnumParams &parameters) = 0;

		/**
		 * Destructor
		 */
		virtual ~IIqrfDb() {}

		/**
		 * Resets network devices database
		 */
		virtual void resetDatabase() = 0;

		/**
		 * Reloads all drivers
		 */
		virtual void reloadDrivers() = 0;

		/**
		 * Reloads coordinator drivers on demand
		 */
		virtual void reloadCoordinatorDrivers() = 0;

		///// Binary Output API

		virtual uint32_t inseryBinaryOutput(BinaryOutput &binaryOutput) = 0;

		virtual void updateBinaryOutput(BinaryOutput &binaryOutput) = 0;

		virtual void removeBinaryOutput(const uint32_t &deviceId) = 0;

		virtual std::unique_ptr<BinaryOutput> getBinaryOutput(const uint32_t &deviceId) = 0;

		virtual std::unique_ptr<BinaryOutput> getBinaryOutputByDeviceId(const uint32_t &deviceId) = 0;

		virtual std::set<uint8_t> getBinaryOutputAddresses() = 0;

		virtual std::map<uint8_t, uint8_t> getBinaryOutputCountMap() = 0;

		///// DALI API

		virtual uint32_t insertDali(Dali &dali) = 0;

		virtual void removeDali(const uint32_t &deviceId) = 0;

		virtual std::unique_ptr<Dali> getDali(const uint32_t &id) = 0;

		virtual std::unique_ptr<Dali> getDaliByDeviceId(const uint32_t &deviceId) = 0;

		virtual std::set<uint8_t> getDaliAddresses() = 0;

		///// Devices API

		virtual uint32_t insertDevice(Device &device) = 0;

		virtual void updateDevice(Device &device) = 0;

		virtual void removeDevice(const uint32_t &id) = 0;

		virtual std::vector<Device> getDevices() = 0;

		virtual std::unique_ptr<Device> getDevice(const uint32_t &id) = 0;

		virtual std::unique_ptr<Device> getDevice(const uint8_t &address) = 0;

		virtual bool deviceImplementsPeripheral(const uint32_t &id, int16_t peripheral) = 0;

		virtual std::set<uint8_t> getDeviceAddresses() = 0;

		virtual uint16_t getDeviceHwpid(const uint8_t &address) = 0;

		virtual uint32_t getDeviceMid(const uint8_t &address) = 0;

		virtual std::string getDeviceMetadata(const uint8_t &address) = 0;

		virtual rapidjson::Document getDeviceMetadataDoc(const uint8_t &address) = 0;

		virtual void setDeviceMetadata(const uint8_t &address, const std::string &metadata) = 0;

		virtual std::vector<DeviceTuple> getDevicesWithProductInfo(std::vector<uint8_t> requestedDevices = {}) = 0;

		///// Device sensors API

		virtual void insertDeviceSensor(DeviceSensor &deviceSensor) = 0;

		virtual void updateDeviceSensor(DeviceSensor &deviceSensor) = 0;

		virtual void removeDeviceSensors(const uint8_t &address) = 0;

		virtual std::unique_ptr<DeviceSensor> getDeviceSensor(const uint32_t &id) = 0;

		virtual std::unique_ptr<DeviceSensor> getDeviceSensor(const uint8_t &address, const uint32_t &sensorId, const uint8_t &index) = 0;

		virtual std::unique_ptr<DeviceSensor> getDeviceSensorByGlobalIndex(const uint8_t &address, const uint8_t &index) = 0;

		virtual std::unique_ptr<DeviceSensor> getDeviceSensorByTypeIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) = 0;

		virtual std::map<uint8_t, std::vector<std::tuple<DeviceSensor, Sensor>>> getDeviceSensorMap() = 0;

		virtual void setDeviceSensorMetadata(DeviceSensor &deviceSensor, nlohmann::json &metadata, std::shared_ptr<std::string> updated) = 0;

		virtual void setDeviceSensorMetadata(const uint8_t &address, const uint8_t &index, nlohmann::json &metadata, std::shared_ptr<std::string> updated) = 0;

		// FRC variant
		virtual void setDeviceSensorMetadata(const uint8_t &address, const uint8_t &type, const uint8_t &index, nlohmann::json &metadata, std::shared_ptr<std::string> updated) = 0;

		virtual void setDeviceSensorValue(DeviceSensor &deviceSensor, double &value, std::shared_ptr<std::string> timestamp) = 0;

		virtual void setDeviceSensorValue(const uint8_t &address, const uint8_t &index, double &value, std::shared_ptr<std::string> timestamp) = 0;

		// FRC variant
		virtual void setDeviceSensorValue(const uint8_t &address, const uint8_t &type, const uint8_t &index, double &value, std::shared_ptr<std::string> timestamp) = 0;

		virtual void updateDeviceSensorValues(const std::map<uint8_t, std::vector<sensor::item::Sensor>> &devices) = 0;

		virtual void updateDeviceSensorValues(const uint8_t &address, const std::string &sensors) = 0;

		// FRC variant
		virtual void updateDeviceSensorValues(const uint8_t &type, const uint8_t &index, const std::set<uint8_t> &selectedNodes, const std::string &sensors) = 0;

		virtual uint8_t getDeviceSensorGlobalIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) = 0;

		///// Drivers API

		virtual uint32_t insertDriver(Driver &driver) = 0;

		virtual void updateDriver(Driver &driver) = 0;

		virtual void removeDriver(const uint32_t &id) = 0;

		virtual std::unique_ptr<Driver> getDriver(const int16_t &peripheral, const double &version) = 0;

		virtual std::vector<Driver> getDriversByProduct(const uint32_t &productId) = 0;

		virtual std::vector<uint32_t> getDriverIdsByProduct(const uint32_t &productId) = 0;

		virtual std::vector<Driver> getLatestDrivers() = 0;

		///// Light API

		virtual uint32_t insertLight(Light &light) = 0;

		virtual void updateLight(Light &light) = 0;

		virtual void removeLight(const uint32_t &deviceId) = 0;

		virtual std::unique_ptr<Light> getLight(const uint32_t &id) = 0;

		virtual std::unique_ptr<Light> getLightByDeviceId(const uint32_t &deviceId) = 0;

		virtual std::set<uint8_t> getLightAddresses() = 0;

		virtual std::map<uint8_t, uint8_t> getLightCountMap() = 0;

		///// Products API

		virtual uint32_t insertProduct(Product &product) = 0;

		virtual void updateProduct(Product &product) = 0;

		virtual void removeProduct(const uint32_t &id) = 0;

		virtual std::unique_ptr<Product> getProduct(const uint32_t &id) = 0;

		virtual std::unique_ptr<Product> getProduct(const uint16_t &hwpid, const uint16_t &hwpidVer, const uint16_t &osBuild, const uint16_t &dpa) = 0;

		virtual uint32_t getCoordinatorProductId() = 0;

		virtual std::vector<uint8_t> getProductDeviceAddresses(const uint32_t &productId) = 0;

		virtual std::string getProductCustomDriver(const uint32_t &productId) = 0;

		///// Product drivers API

		virtual void insertProductDriver(ProductDriver &productDriver) = 0;

		virtual void removeProductDriver(const uint32_t &productId, const uint32_t &driverId) = 0;

		virtual std::set<uint32_t> getProductDriversIds(const uint32_t &productId) = 0;

		virtual std::map<uint32_t, std::set<uint32_t>> getProductsDriversIdMap() = 0;

		///// Sensors API

		virtual uint32_t insertSensor(Sensor &sensor) = 0;

		virtual void updateSensor(Sensor &sensor) = 0;

		virtual void removeSensor(const uint32_t &id) = 0;

		virtual std::unique_ptr<Sensor> getSensor(const uint8_t &type, const std::string &name) = 0;

		virtual std::map<uint8_t, Sensor> getSensorsImplementedByDeviceMap(const uint8_t &address) = 0;

		///// Other API

		virtual std::map<uint16_t, std::set<uint8_t>> getHwpidAddrsMapImplementingSensor(const uint8_t &type) = 0;

		virtual SensorDataSelectMap getSensorDataSelectMap() = 0;

		/**
		 * Checks if metadata should be added to messages
		 * @return true if metadata should be added to messages, false otherwise
		 */
		virtual bool addMetadataToMessage() = 0;

		/// Enumeration handler type
		typedef std::function<void(EnumerationProgress)> EnumerationHandler;

		/**
		 * Register enumeration handler
		 * @param clientId Handler owner
		 * @param handler Handler function
		 */
		virtual void registerEnumerationHandler(const std::string &clientId, EnumerationHandler handler) = 0;

		/**
		 * Unregister enumeration handler
		 * @param clientId Handler owner
		 */
		virtual void unregisterEnumerationHandler(const std::string &clientId) = 0;
	};
}
