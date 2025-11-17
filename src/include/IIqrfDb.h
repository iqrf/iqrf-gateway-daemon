/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#include "EmbedNode.h"
#include "rapidjson/document.h"
#include "JsDriverSensor.h"

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "repositories/binary_output_repo.hpp"
#include "repositories/device_repo.hpp"
#include "repositories/device_sensor_repo.hpp"
#include "repositories/driver_repo.hpp"
#include "repositories/light_repo.hpp"
#include "repositories/migration_repo.hpp"
#include "repositories/product_driver_repo.hpp"
#include "repositories/product_repo.hpp"
#include "repositories/sensor_repo.hpp"

#define PERIPHERAL_LIGHT 74
#define PERIPHERAL_BINOUT 75
#define PERIPHERAL_SENSOR 94

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
		 * Enumeration errors class
		 */
		class EnumerationError {
		public:
			/**
			 * Enumeration errors enum
			 */
			enum Errors {
				AlreadyRunning = -1,
			};

			/**
			 * Base constructor
			 */
			EnumerationError() {}

			/**
			 * Full constructor
			 * @param error Enumeration error
			 */
			EnumerationError(Errors error) : error(error) {};

			/**
			 * Returns enumeration error
			 * @return Enumeration error
			 */
			Errors getError() { return error; }

			/**
			 * Returns message corresponding to the error
			 * @param error Error
			 * @return Error message
			 */
			std::string getErrorMessage() { return errorMessages[error]; }
		private:
			/// Enumeration Error
			Errors error = Errors::AlreadyRunning;
			/// Map of enumeration errors and messages
			std::map<Errors, std::string> errorMessages = {
				{Errors::AlreadyRunning, "Enumeration is already in progress."},
			};
		};

		/// Enumeration handler type
		typedef std::function<void(EnumerationProgress)> EnumerationHandler;

		/**
		 * Check if enumeration is in progress
		 * @return true if enumeration is in progress, false otherwise
		 */
		virtual bool isRunning() = 0;

		/**
		 * Runs enumeration
		 * @param parameters Enumeration parameters
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

		///// BINARY OUTPUT API

		/**
		 * Return binary output entity by device ID
		 * @param deviceId Device ID
		 * @return Binary output entity
		 */
		virtual std::unique_ptr<BinaryOutput> getBinaryOutputByDeviceId(const uint32_t deviceId) = 0;

		/**
		 * Get map of device addresses and number of implemented binary outputs
		 * @return Map of device addresses and number of implemented binary outputs
		 */
		virtual std::map<uint8_t, uint8_t> getBinaryOutputCountMap() = 0;

		///// DEVICE API

		/**
		 * Returns device by address
		 * @param addr Device address
		 * @return Device
		 */
		virtual std::unique_ptr<Device> getDeviceByAddress(const uint8_t address) = 0;

		/**
		 * Returns device by module ID
		 * @param mid Module ID
		 * @return Device
		 */
		virtual std::unique_ptr<Device> getDeviceByMid(const uint32_t mid) = 0;

		/**
		 * Retrieves information about devices in network
		 * @return Vector of devices and their products
		 */
		virtual std::vector<std::pair<Device, Product>> getDevices(const std::vector<uint8_t>& requestedDevices = {}) = 0;

		/**
		 * Update device record
		 * @param device Device entity
		 */
		virtual void updateDevice(Device &device) = 0;

		/**
		 * Returns addresses of devices in network from database
		 * @return std::set<uint8_t> Set of device addresses
		 */
		virtual std::set<uint8_t> getDeviceAddresses() = 0;

		/**
		 * Retrieves device MID specified by address
		 * @param address Device address
		 * @return Device MID
		 */
		virtual std::optional<uint32_t> getDeviceMid(const uint8_t address) = 0;

		/**
		 * Retrieves device HWPID specified by address
		 * @param address Device address
		 * @return Device HWPID
		 */
		virtual std::optional<uint16_t> getDeviceHwpid(const uint8_t address) = 0;

		/**
		 * Check if device implements peripheral
		 * @param deviceId Device ID
		 * @param peripheral Peripheral
		 * @return `true` if Device implements peripheral, `false` otherwise
		 */
		virtual bool deviceImplementsPeripheral(const uint32_t &deviceId, const int16_t peripheral) = 0;

		/**
		 * Retrieves metadata stored at device specified by address
		 * @param address Device address
		 * @return Device metadata
		 */
		virtual std::shared_ptr<std::string> getDeviceMetadata(const uint8_t address) = 0;

		/**
		 * Retrieves metadata stored at device specified by address in a rapidjson document
		 * @param address Device address
		 * @return Device metadata document
		 */
		virtual rapidjson::Document getDeviceMetadataDoc(const uint8_t address) = 0;

		/**
		 * Sets metadata to device at specified address
		 * @param address Device address
		 * @param metadata Metadata to store
		 */
		virtual void setDeviceMetadata(const uint8_t address, std::shared_ptr<std::string> metadata) = 0;

		/**
		 * Retrieves node map of node addresses, and their hwpids and MIDs
		 * @return Map of node addresses, and their hwpids and MIDs
		 */
		virtual std::map<uint8_t, embed::node::NodeMidHwpid> getNodeMidHwpidMap() = 0;

		///// DEVICE SENSOR API

		/**
		 * Retrieves map of device addresses and vector of sensor index and data
		 * @return Map of device addresses and vector of sensor index and data
		 */
		virtual std::map<uint8_t, std::vector<std::pair<uint8_t, Sensor>>> getDeviceAddressIndexSensorMap() = 0;

		/**
		 * Retrieves map of device addresses and vector of device sensors and sensors
		 * @return Map of device addresses and vector of device sensors and sensors
		 */
		virtual std::map<uint8_t, std::vector<std::pair<DeviceSensor, Sensor>>> getDeviceAddressSensorMap() = 0;

		/**
		 * Constructs and returns a map of sensor types, devices that implement them and their local indexes
		 * @return Map of sensor types and devices
		 */
		virtual std::unordered_map<uint8_t, std::vector<std::pair<uint8_t, uint8_t>>> getSensorTypeAddressIndexMap() = 0;

		/**
		 * Retrieves global sensor index from address, type and type index
		 * @param address Device address
		 * @param type Sensor type
		 * @param index Type index
		 * @return Global sensor index
		 */
		virtual std::optional<uint8_t> getGlobalSensorIndex(const uint8_t address, const uint8_t type, const uint8_t typeIndex) = 0;

		/**
		 * Returns map of hwpids and devices implementing sensor device specified by type and index
		 * @param type Sensor type
		 * @return Map of hwpids and device addresses
		 */
		virtual std::map<uint16_t, std::set<uint8_t>> getSensorDeviceHwpidAddressMap(const uint8_t type) = 0;

		///// LIGHT API

		/**
		 * Get addresses of devices implementing light
		 * @return Set of device addresses
		 */
		virtual std::set<uint8_t> getLightAddresses() = 0;

		///// PRODUCT API

    /**
     * Returns product entity by ID
     * @param productId Product ID
     * @return Product
     */
		virtual std::unique_ptr<Product> getProduct(const uint32_t productId) = 0;

		///// SENSOR API

		/**
		 * Get sensor entity by device address, global index and sensor type
		 * @param address Device address
		 * @param index Global index
		 * @param type Sensor type
		 * @return Sensor entity
		 */
		virtual std::unique_ptr<Sensor> getSensorByAddressIndexType(const uint8_t address, const uint8_t index,
			const uint8_t type) = 0;

		/**
		 * Returns map of device sensor indexes and sensor entities
		 * @param address Device address
		 * @return Map of device sensor indexes and sensor entities
		 */
		virtual std::map<uint8_t, Sensor> getDeviceSensorsMapByAddress(const uint8_t address) = 0;

		//// OTHER API

		/**
		 * Updates sensor values from map of addresses and sensor objects
		 * @param devices Map of devices and sensors
		 */
		virtual void updateSensorValues(const std::map<uint8_t, std::vector<sensor::item::Sensor>> &devices) = 0;

		/**
		 * Updates sensor values from_ReadSensorsWithTypes response
		 * @param address Device address
		 * @param sensors Parsed sensors JSON string
		 */
		virtual void updateSensorValues(const uint8_t &address, const std::string &sensors) = 0;

		/**
		 * Updates sensor values from Sensor_Frc response
		 * @param type Sensor type
		 * @param index Sensor index
		 * @param selectedNodes Set of selected nodes
		 * @param sensors Parsed sensors JSON string
		 */
		virtual void updateSensorValues(const uint8_t &type, const uint8_t &index, const std::set<uint8_t> &selectedNodes, const std::string &sensors) = 0;

		/**
		 * Checks if metadata should be added to messages
		 * @return true if metadata should be added to messages, false otherwise
		 */
		virtual bool getMetadataToMessages() = 0;

		/**
		 * Sets including metadata to messages
		 * @param includeMetadata Metadata inclusion setting
		 */
		virtual void setMetadataToMessages(bool includeMetadata) = 0;

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

		/**
		 * Get quantity by type from cache
		 * @param type Sensor type
		 * @return Cache quantity
		 */
		virtual std::shared_ptr<IJsCacheService::Quantity> getQuantityByType(const uint8_t type) = 0;
	};
}
