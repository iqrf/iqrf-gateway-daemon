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

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include "IIqrfDb.h"
#include "IqrfDbAux.h"

#include "IIqrfDpaService.h"
#include "IJsCacheService.h"
#include "IJsRenderService.h"
#include "ILaunchService.h"
#include "ITraceService.h"
#include "JsDriverSensor.h"
#include "ShapeProperties.h"
#include "Trace.h"

#include <openssl/evp.h>
#include <nlohmann/json.hpp>
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/rapidjson.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include "Common.h"

using namespace iqrf::db::models;

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#define EEEPROM_READ_MAX_LEN 54

typedef std::shared_ptr<Product> ProductPtr;
typedef std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> UniqueProduct;
typedef std::tuple<uint16_t, uint16_t> HwpidTuple;
typedef std::tuple<uint16_t, std::string> OsTuple;
typedef std::tuple<Device, ProductPtr> DeviceProduct;

namespace iqrf {
	/**
	 * IQRF DB
	 */
	class IqrfDb : public IIqrfDb {
	public:
		/**
		 * Constructor
		 */
		IqrfDb();

		/**
		 * Destructor
		 */
		virtual ~IqrfDb();

		/**
		 * Check if enumeration is in progress
		 * @return true if enumeration is in progress, false otherwise
		 */
		bool isRunning() override;

		/**
		 * Performs network enumeration according to request parameters
		 * @param parameters Enumeration parameters
		 */
		void enumerate(IIqrfDb::EnumParams &parameters) override;

		/**
		 * Resets IQRF Database
		 */
		void resetDatabase() override;

		/**
		 * Reloads all drivers
		 */
		void reloadDrivers() override;

		/**
		 * Reloads coordinator drivers on-demand
		 */
		void reloadCoordinatorDrivers() override;

		///// BINARY OUTPUT API

		/**
		 * Return binary output entity by ID
		 * @param id Binary output ID
		 * @return Binary output entity
		 */
		std::unique_ptr<BinaryOutput> getBinaryOutput(const uint32_t id) override;

		/**
		 * Return binary output entity by device ID
		 * @param deviceId Device ID
		 * @return Binary output entity
		 */
		std::unique_ptr<BinaryOutput> getBinaryOutputByDeviceId(const uint32_t deviceId) override;

		/**
		 * Insert binary output record
		 * @param binaryOutput Binary output entity
		 * @return Binary output ID
		 */
		uint32_t insertBinaryOutput(BinaryOutput &binaryOutput) override;

		/**
		 * Update binary output record
		 * @param binaryOutput Binary output entity
		 */
		void updateBinaryOutput(BinaryOutput &binaryOutput) override;

		/**
		 * Remove binary output record by ID
		 * @param id Binary output ID
		 */
		void removeBinaryOutput(const uint32_t id) override;

		/**
		 * Remove binary output record by device ID
		 * @param deviceId Device ID
		 */
		void removeBinaryOutputByDeviceId(const uint32_t deviceId) override;

		/**
		 * Get addresses of devices implementing binary output
		 * @return Set of device addresses
		 */
		std::set<uint8_t> getBinaryOutputAddresses() override;

		/**
		 * Get map of device addresses and number of implemented binary outputs
		 * @return Map of device addresses and number of implemented binary outputs
		 */
		std::map<uint8_t, uint8_t> getBinaryOutputCountMap() override;

		///// DEVICE API

		/**
		 * Returns device by address
		 * @param addr Device address
		 * @return Device
		 */
		std::unique_ptr<Device> getDeviceByAddress(const uint8_t address) override;

		/**
		 * Returns device by module ID
		 * @param mid Module ID
		 * @return Device
		 */
		std::unique_ptr<Device> getDeviceByMid(const uint32_t mid) override;

		/**
		 * Returns vector of device and product pairs
		 * @return Vector of device and product pairs
		 */
		std::vector<std::pair<Device, Product>> getDevices(const std::vector<uint8_t>& requestedDevices = {}) override;

		/**
		 * Update device record
		 * @param device Device entity
		 */
		void updateDevice(Device &device) override;

		/**
		 * Returns addresses of devices in network from database
		 * @return std::set<uint8_t> Set of device addresses
		 */
		std::set<uint8_t> getDeviceAddresses() override;

		/**
		 * Retrieves device MID specified by address
		 * @param address Device address
		 * @return Device MID
		 */
		std::optional<uint32_t> getDeviceMid(const uint8_t address) override;

		/**
		 * Retrieves device HWPID specified by address
		 * @param address Device address
		 * @return Device HWPID
		 */
		std::optional<uint16_t> getDeviceHwpid(const uint8_t address) override;

		/**
		 * Check if device implements peripheral
		 * @param deviceId Device ID
		 * @param peripheral Peripheral
		 * @return `true` if Device implements peripheral, `false` otherwise
		 */
		bool deviceImplementsPeripheral(const uint32_t &deviceId, const int16_t peripheral) override;

		/**
		 * Retrieves metadata stored at device specified by address
		 * @param address Device address
		 * @return Device metadata
		 */
		std::shared_ptr<std::string> getDeviceMetadata(const uint8_t address) override;

		/**
		 * Retrieves metadata stored at device specified by address in a rapidjson document
		 * @param address Device address
		 * @return Device metadata document
		 */
		rapidjson::Document getDeviceMetadataDoc(const uint8_t address) override;

		/**
		 * Sets metadata to device at specified address
		 * @param address Device address
		 * @param metadata Metadata to store
		 */
		void setDeviceMetadata(const uint8_t address, std::shared_ptr<std::string> metadata) override;

		/**
		 * Retrieves node map of node addresses, and their hwpids and MIDs
		 * @return Map of node addresses, and their hwpids and MIDs
		 */
		std::map<uint8_t, embed::node::NodeMidHwpid> getNodeMidHwpidMap() override;

		///// DEVICE SENSOR API

		/**
		 * Checks if device has sensors
		 * @param address Device address
		 * @return `true` if device has sensors, `false` otherwise
		 */
		bool deviceHasSensors(const uint8_t address) override;

		/**
		 * Retrieves map of device addresses and vector of sensor index and data
		 * @return Map of device addresses and vector of sensor index and data
		 */
		std::map<uint8_t, std::vector<std::pair<uint8_t, Sensor>>> getDeviceAddressIndexSensorMap() override;

		/**
		 * Retrieves map of device addresses and vector of device sensors and sensors
		 * @return Map of device addresses and vector of device sensors and sensors
		 */
		std::map<uint8_t, std::vector<std::pair<DeviceSensor, Sensor>>> getDeviceAddressSensorMap() override;

		/**
		 * Constructs and returns a map of sensor types, devices that implement them and their local indexes
		 * @return Map of sensor types and devices
		 */
		std::unordered_map<uint8_t, std::vector<std::pair<uint8_t, uint8_t>>> getSensorTypeAddressIndexMap() override;

		/**
		 * Retrieves global sensor index from address, type and type index
		 * @param address Device address
		 * @param type Sensor type
		 * @param index Type index
		 * @return Global sensor index
		 */
		std::optional<uint8_t> getGlobalSensorIndex(const uint8_t address, const uint8_t type, const uint8_t index) override;

		/**
		 * Returns map of hwpids and devices implementing sensor device specified by type and index
		 * @param type Sensor type
		 * @return Map of hwpids and device addresses
		 */
		std::map<uint16_t, std::set<uint8_t>> getSensorDeviceHwpidAddressMap(const uint8_t type) override;

		/**
		 * Stores value of sensor
		 * @param address Device address
		 * @param type Sensor type
		 * @param index Sensor index
		 * @param value Last measured value
		 * @param updated Last updated
		 * @param frc Data from FRC response
		 */
		void setDeviceSensorValue(const uint8_t address, const uint8_t type, const uint8_t index, const double value,
			std::shared_ptr<std::string> updated, bool frc) override;

		/**
		 * Stores value of sensor as metadata (for non-atomic values)
		 * @param address Device address
		 * @param type Sensor type
		 * @param index Sensor index
		 * @param metadata Last measured value
		 * @param updated Last updated
		 * @param frc Data from FRC response
		 */
		void setDeviceSensorMetadata(const uint8_t address, const uint8_t type, const uint8_t index, json &metadata,
			std::shared_ptr<std::string> updated, bool frc);

		///// LIGHT API

		/**
		 * Return light entity by ID
		 * @param id Light ID
		 * @return Light entity
		 */
		std::unique_ptr<Light> getLight(const uint32_t id) override;

		/**
		 * Return light entity by device ID
		 * @param deviceId Device ID
		 * @return Light entity
		 */
		std::unique_ptr<Light> getLightByDeviceId(const uint32_t deviceId) override;

		/**
		 * Insert light record
		 * @param binaryOutput Light entity
		 * @return Light ID
		 */
		uint32_t insertLight(Light &light) override;

		/**
		 * Update light record
		 * @param binaryOutput Light entity
		 */
		void updateLight(Light &light) override;

		/**
		 * Remove light record by ID
		 * @param id Light ID
		 */
		void removeLight(const uint32_t id) override;

		/**
		 * Remove light record by device ID
		 * @param deviceId Device ID
		 */
		void removeLightByDeviceId(const uint32_t deviceId) override;

		/**
		 * Get addresses of devices implementing light
		 * @return Set of device addresses
		 */
		std::set<uint8_t> getLightAddresses() override;

		///// PRODUCT API

		/**
		 * Get product entity by ID
		 * @param productId Product ID
		 * @return
		 */
		std::unique_ptr<Product> getProduct(const uint32_t productId) override;

		///// SENSOR API

		/**
		 * Get sensor entity by device address, global index and sensor type
		 * @param address Device address
		 * @param index Global index
		 * @param type Sensor type
		 * @return Sensor entity
		 */
		std::unique_ptr<Sensor> getSensorByAddressIndexType(const uint8_t address, const uint8_t index,
			const uint8_t type) override;

		/**
		 * Returns map of device sensor indexes and Sensor entities
		 * @param address Device address
		 */
		std::map<uint8_t, Sensor> getDeviceSensorsMapByAddress(const uint8_t address) override;

		/**
		 * Returns map of device sensor indexes and sensor IDs
		 * @param address Device address
		 */
		std::map<uint8_t, uint32_t> getDeviceSensorsIdMapByAddress(const uint8_t address) override;

		///// OTHER API

		/**
		 * Updates sensor values from map of addresses and sensor objects
		 * @param devices Map of devices and sensors
		 */
		void updateSensorValues(const std::map<uint8_t, std::vector<sensor::item::Sensor>> &devices) override;

		/**
		 * Updates sensor values from_ReadSensorsWithTypes response
		 * @param address Device address
		 * @param sensors Parsed sensors JSON string
		 */
		void updateSensorValues(const uint8_t &address, const std::string &sensors) override;

		/**
		 * Updates sensor values from Sensor_Frc response
		 * @param type Sensor type
		 * @param index Sensor index
		 * @param selectedNodes Set of selected nodes
		 * @param sensors Parsed sensors JSON string
		 */
		void updateSensorValues(const uint8_t &type, const uint8_t &index, const std::set<uint8_t> &selectedNodes, const std::string &sensors) override;

		/**
		 * Checks if metadata should be added to messages
		 * @return true if metadata should be added to messages, false otherwise
		 */
		bool getMetadataToMessages() override;

		/**
		 * Sets including metadata to messages
		 * @param includeMetadata Metadata inclusion setting
		 */
		void setMetadataToMessages(bool includeMetadata) override;

		/**
		 * Register enumeration handler
		 * @param clientId Handler owner
		 * @param handler Handler function
		 */
		void registerEnumerationHandler(const std::string &clientId, EnumerationHandler handler) override;

		/**
		 * Unregister enumeration handler
		 * @param clientId Handler owner
		 */
		void unregisterEnumerationHandler(const std::string &clientId) override;

		/**
		 * Get quantity by type from cache
		 * @param type Sensor type
		 * @return Cache quantity
		 */
		std::shared_ptr<IJsCacheService::Quantity> getQuantityByType(const uint8_t type) override;

		/**
		 * Component instance lifecycle activate step
		 * @param props Component instance properties
		 */
		void activate(const shape::Properties *props = 0);

		/**
		 * Component instance lifecycle modify step
		 * @param props Component instance properties
		 */
		void modify(const shape::Properties *props);

		/**
		 * Component instance lifecycle deactivate step
		 */
		void deactivate();

		/**
		 * Attaches DPA service interface
		 * @param iface DPA service interface
		 */
		void attachInterface(IIqrfDpaService *iface);

		/**
		 * Detaches DPA service interface
		 * @param iface DPA service interface
		 */
		void detachInterface(IIqrfDpaService *iface);

		/**
		 * Attaches JS cache service interface
		 * @param iface JS cache service interface
		 */
		void attachInterface(IJsCacheService *iface);

		/**
		 * Detaches JS cache service interface
		 * @param iface JS cache service interface
		 */
		void detachInterface(IJsCacheService *iface);

		/**
		 * Attaches JS render service interface
		 * @param iface JS render service interface
		 */
		void attachInterface(IJsRenderService *iface);

		/**
		 * Detaches JS render service interface
		 * @param iface JS render service interface
		 */
		void detachInterface(IJsRenderService *iface);

		/**
		 * Attaches Launch service interface
		 * @param iface Launch service interface
		 */
		void attachInterface(shape::ILaunchService *iface);

		/**
		 * Detaches Launch service interface
		 * @param iface Launch service interface
		 */
		void detachInterface(shape::ILaunchService *iface);

		/**
		 * Attaches Tracing service interface
		 * @param iface Tracing service interface
		 */
		void attachInterface(shape::ITraceService *iface);

		/**
		 * Dettaches Tracing service interface
		 * @param iface Tracing service interface
		 */
		void detachInterface(shape::ITraceService *iface);

		/// Node memory address for FRC
		static const uint16_t m_memoryAddress = 0x04a0;
	private:
		/**
		 * Initializes database
		 */
		void initializeDatabase();

		/**
		 * Detect and apply migrations
		 */
		void migrateDatabase();

		/**
		 * Execute migrations
		 * @param migration Migration name
		 */
		void executeMigration(const std::string &migration);

		/**
		 * Assign product names if product doesn't have a name and is available in cache
		 */
		void updateDbProductNames();

		/**
		 * Compare DB driver hashes with cache driver hashes and update drivers if changes are detected
		 */
		void updateDbDrivers();

		/**
		 * Starts enumeration thread
		 * @param parameters Enumeration parameters
		 */
		void startEnumerationThread(IIqrfDb::EnumParams &parameters);

		/**
		 * Stops enumeration thread
		 */
		void stopEnumerationThread();

		/**
		 * Analyzes DPA responses and triggers enumeration if network altering action has been performed.
		 * @param message DPA response
		 */
		void analyzeDpaMessage(const DpaMessage &message);

		/**
		 * Runs enumeration
		 * @param parameters Enumeration parameters
		 */
		void runEnumeration(IIqrfDb::EnumParams &parameters);

		/**
		 * Sends enumeration progress response
		 * @param progress Enumeration stage
		 */
		void sendEnumerationResponse(EnumerationProgress progress);

		/**
		 * Compares network with database information, selects nodes to enumerate and retrieves their routing information
		 * @param reenumerate Force re-enumerate existing records in database
		 */
		void checkNetwork(bool reenumerate);

		/**
		 * Performs TR devices enumeration
		 */
		void enumerateDevices();

		/**
		 * Performs [C] device enumeration
		 */
		void coordinatorEnumeration();

		/**
		 * Perform enumeration by polling devices
		 */
		void pollEnumeration();

		/**
		 * Perform enumeration utilizing FRC commands
		 */
		void frcEnumeration();

		/**
		 * Sends EEEPROM read request and stores response data in a buffer
		 * @param data EEEPROM data buffer
		 * @param address EEEPROM address to read from
		 * @param len Number of bytes to read
		 */
		void eeepromRead(uint8_t* data, const uint16_t &address, const uint8_t &len);

		/**
		 * Performs FRC requests for device HWPID and HWPID version
		 * @param hwpidMap Map of device addresses and their respective HWPID and HWPID version
		 * @param frcCount Number of FRC requests to send
		 * @param nodes Number of nodes to read per FRC request
		 * @param remainingNodes Number of nodes to read in the final FRC request
		 */
		void frcHwpid(std::map<uint8_t, HwpidTuple> *hwpidMap, uint8_t &frcCount, uint8_t &nodes, uint8_t &remainingNodes);

		/**
		 * Performs FRC requests for device DPA version
		 * @param dpaMap Map of device addresses and their respective DPA versions
		 * @param frcCount Number of FRC requests to send
		 * @param nodes Number of nodes to read per FRC request
		 * @param remainingNodes Number of nodes to read in the final FRC request
		 */
		void frcDpa(std::map<uint8_t, uint16_t> *dpaMap, uint8_t &frcCount, uint8_t &nodes, uint8_t &remainingNodes);

		/**
		 * Performs FRC requests for device OS build and version
		 * @param osMap Map of device addresses and their respective OS builds and versions
		 * @param frcCount Number of FRC requests to send
		 * @param nodes Number of nodes to read per FRC request
		 * @param remainingNodes Number of nodes to read in the final FRC request
		 */
		void frcOs(std::map<uint8_t, OsTuple> *osMap, uint8_t &frcCount, uint8_t &nodes, uint8_t &remainingNodes);

		/**
		 * Performs FRC request to ping online nodes
		 * @return Set of online nodes addresses
		 */
		const std::set<uint8_t> frcPing();

		/**
		 * Performs FRC send selective and stores response data in a buffer
		 * @param data FRC data buffer
		 * @param address Memory address to read from
		 * @param pnum Peripheral number
		 * @param pcmd Peripheral command
		 * @param numNodes Number of nodes to read from
		 * @param processedNodes Number of already read nodes
		 */
		void frcSendSelectiveMemoryRead(uint8_t* data, const uint16_t &address, const uint8_t &pnum, const uint8_t &pcmd, const uint8_t &numNodes, const uint8_t &processedNodes);

		/**
		 * Performs FRC extra result request and stores response data in a buffer
		 * @param data FRC data buffer
		 */
		void frcExtraResult(uint8_t* data);

		std::set<int> getEmbeddedStandardPeripherals(const uint8_t &addr);

		/**
		 * Performs product package enumeration
		 */
		void productPackageEnumeration();

		/**
		 * Enumerates non-certified / development / example products from network
		 * @param addr Device address
		 */
		void enumerateNoncertifiedProduct(const uint8_t &addr);

		/**
		 * Updates products in database
		 */
		void updateDatabaseProducts();

		/**
		 * Performs standard device enumeration
		 */
		void standardEnumeration();

		/**
		 * Performs binary output standard enumeration
		 * @param deviceId Device ID
		 * @param address Device address
		 */
		void binoutEnumeration(const uint32_t &deviceId, const uint8_t &address);

		/**
		 * Performs light standard enumeration
		 * @param deviceId Device ID
		 * @param address Device address
		 */
		void lightEnumeration(const uint32_t &deviceId);

		/**
		 * Performs sensor standard enumeration
		 * @param address Device address
		 */
		void sensorEnumeration(const uint8_t &address);

		/**
		 * Retrieves bonded devices
		 */
		void getBondedNodes();

		/**
		 * Retrieves discovered devices
		 * @param bondedNodes Vector of bonded nodes for filtering
		 */
		void getDiscoveredNodes();

		/**
		 * Retrieves MIDs of bonded devices
		 */
		void getMids();

		/**
		 * Retrieves VRN, zone and parent information
		 */
		void getRoutingInformation();

		/**
		 * Loads default drivers based on coordinator information
		 */
		void loadCoordinatorDrivers();

		/**
		 * Loads drivers coresponding to the products
		 */
		void loadProductDrivers();

		std::string generateDriverHash(const std::string &driver);

		/**
		 * Loads DaemonWrapper code
		 */
		std::string loadWrapper();

		/**
		 * Clears auxiliary buffers holding device information
		 */
		void clearAuxBuffers();

		/**
		 * Waits for and claims exclusive access when available
		 */
		void waitForExclusiveAccess();

		/**
		 * Resets exclusive access
		 */
		void resetExclusiveAccess();

		/// Component instance name
		std::string m_instance;
		/// Path to directory containing migration files
		std::string m_migrationDir;
		/// Path to database file
		std::string m_dbPath;
		/// Path to daemon js wrapper
		std::string m_wrapperPath;
		/// Database accessor
		std::shared_ptr<SQLite::Database> m_db = nullptr;
		/// DPA service
		IIqrfDpaService *m_dpaService = nullptr;
		/// Enumeration condition variable
		std::condition_variable m_exclusiveAccessCv;
		/// Exclusive access
		std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
		/// JS cache service
		IJsCacheService *m_cacheService = nullptr;
		/// JS render service
		IJsRenderService *m_renderService = nullptr;
		/// Launch service
		shape::ILaunchService *m_launchService = nullptr;
		/// Coordinator parameters
		IIqrfDpaService::CoordinatorParameters m_coordinatorParams;
		/// Set of device addresses to enumerate
		std::set<uint8_t> m_toEnumerate;
		/// Set of discovered device addresses
		std::set<uint8_t> m_discovered;
		/// Set of devices to delete
		std::set<uint8_t> m_toDelete;
		/// Map of device addresses and their respective MIDs
		std::map<uint8_t, uint32_t> m_mids;
		/// Map of device vrns
		std::map<uint8_t, uint8_t> m_vrns;
		/// Map of device zones
		std::map<uint8_t, uint8_t> m_zones;
		/// Map of device parent addresses
		std::map<uint8_t, uint8_t> m_parents;
		/// Map of unique products
		std::map<UniqueProduct, Product> m_productMap;
		/// Map of device addresses and their respective products
		std::map<uint8_t, ProductPtr> m_deviceProductMap;
		/// Enumerate network automatically without initial user invocation
		bool m_autoEnumerateBeforeInvoked = false;
		/// Enumerate network when daemon launches
		bool m_enumerateOnLaunch = false;
		/// Include device metadata in responses
		bool m_metadataToMessages = false;
		/// Controls whether enumeration should run or stop
		std::atomic_bool m_enumRun;
		/// Repeat enumeration in case of a failure
		std::atomic_bool m_enumRepeat;
		/// Run enumeration thread
		std::atomic_bool m_enumThreadRun;
		/// Enumeration condition variable
		std::condition_variable m_enumCv;
		/// Enumeration mutex
		std::mutex m_enumMutex;
		/// Enumeration thread
		std::thread m_enumThread;
		/// Enumeration handler map
		std::map<std::string, EnumerationHandler> m_enumHandlers;
		/// Enumeration parameters
		EnumParams m_params;
	};
}
