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

#include "EmbedNode.h"
#include "rapidjson/document.h"
#include "../IqrfDb/Entities/BinaryOutput.h"
#include "../IqrfDb/Entities/Device.h"
#include "../IqrfDb/Entities/DeviceSensor.h"
#include "../IqrfDb/Entities/Light.h"
#include "../IqrfDb/Entities/Product.h"
#include "../IqrfDb/Entities/Sensor.h"
#include "JsDriverSensor.h"

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <vector>

typedef std::tuple<Device, Product> DeviceProductTuple;
typedef std::tuple<uint8_t, uint8_t> AddrIndex;
typedef std::unordered_map<uint8_t, std::vector<AddrIndex>> SensorSelectMap;


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

		virtual Device getDevice(const uint8_t &addr) = 0;

		/**
		 * Retrieves information about devices in network
		 * @return Vector of devices
		 */
		virtual std::vector<DeviceProductTuple> getDevices(std::vector<uint8_t> requestedDevices = {}) = 0;

		/**
		 * Returns addresses of devices in network from database
		 * @return std::set<uint8_t> Set of device addresses
		 */
		virtual std::set<uint8_t> getDeviceAddrs() = 0;

		virtual Product getProductById(const uint32_t &productId) = 0;

		///// DEVICE PERIPHERAL API

		virtual bool deviceImplementsPeripheral(const uint32_t &deviceId, const int16_t peripheral) = 0;

		///// BINARY OUTPUT API

		/**
		 * Check if a binary output record exists for device
		 * @param deviceId Device ID
		 * @return `true` if a binary output record exists, `false` otherwise
		 */
		virtual bool binaryOutputExists(const uint32_t &deviceId) = 0;

		/**
		 * Return binary output entity by ID
		 * @param id Binary output ID
		 * @return Binary output entity
		 */
		virtual std::unique_ptr<BinaryOutput> getBinaryOutput(const uint32_t &id) = 0;

		/**
		 * Return binary output entity by device ID
		 * @param deviceId Device ID
		 * @return Binary output entity
		 */
		virtual std::unique_ptr<BinaryOutput> getBinaryOutputByDeviceId(const uint32_t &deviceId) = 0;

		/**
		 * Insert binary output record
		 * @param binaryOutput Binary output entity
		 * @return Binary output ID
		 */
		virtual uint32_t insertBinaryOutput(BinaryOutput &binaryOutput) = 0;

		/**
		 * Update binary output record
		 * @param binaryOutput Binary output entity
		 */
		virtual void updateBinaryOutput(BinaryOutput &binaryOutput) = 0;

		/**
		 * Remove binary output record by ID
		 * @param id Binary output ID
		 */
		virtual void removeBinaryOutput(const uint32_t &id) = 0;

		/**
		 * Remove binary output record by device ID
		 * @param deviceId Device ID
		 */
		virtual void removeBinaryOutputByDeviceId(const uint32_t &deviceId) = 0;

		/**
		 * Get addresses of devices implementing binary output
		 * @return Set of device addresses
		 */
		virtual std::set<uint8_t> getBinaryOutputAddresses() = 0;

		/**
		 * Get map of device addresses and number of implemented binary outputs
		 * @return Map of device addresses and number of implemented binary outputs
		 */
		virtual std::map<uint8_t, uint8_t> getBinaryOutputCountMap() = 0;

		///// LIGHT API

		/**
		 * Check if a light record exists for device
		 * @param deviceId Device ID
		 * @return `true` if a light record exists, `false` otherwise
		 */
		virtual bool lightExists(const uint32_t &deviceId) = 0;

		/**
		 * Return light entity by ID
		 * @param id Light ID
		 * @return Light entity
		 */
		virtual std::unique_ptr<Light> getLight(const uint32_t &id) = 0;

		/**
		 * Return light entity by device ID
		 * @param deviceId Device ID
		 * @return Light entity
		 */
		virtual std::unique_ptr<Light> getLightByDeviceId(const uint32_t &deviceId) = 0;

		/**
		 * Insert light record
		 * @param binaryOutput Light entity
		 * @return Light ID
		 */
		virtual uint32_t insertLight(Light &light) = 0;

		/**
		 * Update light record
		 * @param binaryOutput Light entity
		 */
		virtual void updateLight(Light &light) = 0;

		/**
		 * Remove light record by ID
		 * @param id Light ID
		 */
		virtual void removeLight(const uint32_t &id) = 0;

		/**
		 * Remove light record by device ID
		 * @param deviceId Device ID
		 */
		virtual void removeLightByDeviceId(const uint32_t &deviceId) = 0;

		/**
		 * Get addresses of devices implementing light
		 * @return Set of device addresses
		 */
		virtual std::set<uint8_t> getLightAddresses() = 0;

		///// SENSOR API

		virtual std::unique_ptr<Sensor> getSensorByAddrIndexType(const uint8_t &deviceAddress, const uint8_t &index, const uint8_t &type) = 0;

		/**
		 * Checks if device implements sensors
		 * @param deviceAddress Device address
		 * @return true if device implements sensors, false otherwise
		 */
		virtual bool hasSensors(const uint8_t &deviceAddress) = 0;

		/**
		 * Returns map of device sensor indexes and sensor entities
		 * @param deviceAddress Device address
		 * @return Map of device sensor indexes and sensor entities
		 */
		virtual std::map<uint8_t, Sensor> getDeviceSensorsByAddress(const uint8_t &deviceAddress) = 0;

		/**
		 * Returns map of device sensor indexes and sensor IDs
		 * @param deviceAddress Device address
		 * @return Map of device sensor indexes and sensor IDs
		 */
		virtual std::map<uint8_t, uint32_t> getDeviceSensorIndexIdMap(const uint8_t &deviceAddress) = 0;

		/**
		 * Retrieves information about devices implementing Sensor standard
		 * @return Map of device addresses and implemented sensors
		 */
		virtual std::map<uint8_t, std::vector<std::tuple<DeviceSensor, Sensor>>> getSensors() = 0;

		/**
		 * Constructs and returns a map of sensor types, devices that implement them and their local indexes
		 * @return Map of sensor types and devices
		 */
		virtual SensorSelectMap constructSensorSelectMap() = 0;

		/**
		 * Retrieves global sensor index from address, type and type index
		 * @param address Device address
		 * @param type Sensor type
		 * @param index Type index
		 * @return Global sensor index
		 */
		virtual uint8_t getGlobalSensorIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) = 0;

		/**
		 * Stores value of sensor
		 * @param address Device address
		 * @param type Sensor type
		 * @param index Sensor index
		 * @param value Last measured value
		 * @param updated Last updated
		 */
		virtual void setSensorValue(const uint8_t &address, const uint8_t &type, const uint8_t &index, const double &value, std::shared_ptr<std::string> updated) = 0;

		/**
		 * Retrieves device HWPID specified by address
		 * @param address Device address
		 * @return Device HWPID
		 */
		virtual uint16_t getDeviceHwpid(const uint8_t &address) = 0;

		/**
		 * Retrieves device MID specified by address
		 * @param address Device address
		 * @return Device MID
		 */
		virtual uint32_t getDeviceMid(const uint8_t &address) = 0;

		/**
		 * Retrieves metadata stored at device specified by address
		 * @param address Device address
		 * @return Device metadata
		 */
		virtual std::shared_ptr<std::string> getDeviceMetadata(const uint8_t &address) = 0;

		/**
		 * Retrieves metadata stored at device specified by address in a rapidjson document
		 * @param address Device address
		 * @return Device metadata document
		 */
		virtual rapidjson::Document getDeviceMetadataDoc(const uint8_t &address) = 0;

		/**
		 * Sets metadata to device at specified address
		 * @param address Device address
		 * @param metadata Metadata to store
		 */
		virtual void setDeviceMetadata(const uint8_t &address, std::shared_ptr<std::string> metadata) = 0;

		/**
		 * Returns map of hwpids and devices implementing sensor device specified by type and index
		 * @param type Sensor type
		 * @return Map of hwpids and device addresses
		 */
		virtual std::map<uint16_t, std::set<uint8_t>> getSensorDeviceHwpids(const uint8_t &type) = 0;

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

		virtual std::map<uint8_t, embed::node::NodeMidHwpid> getNodeMidHwpidMap() = 0;
	};
}
