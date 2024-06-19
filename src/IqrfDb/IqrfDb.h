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

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
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

#include <nlohmann/json.hpp>
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/rapidjson.h"
#include <sqlite_orm/sqlite_orm.h>
#include "Common.h"
#include "Storage.h"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#define EEEPROM_READ_MAX_LEN 54

#define PERIPHERAL_BINOUT 75
#define PERIPHERAL_DALI 74
#define PERIPHERAL_LIGHT 113
#define PERIPHERAL_SENSOR 94

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

		///// Binary Output API

		uint32_t inseryBinaryOutput(BinaryOutput &binaryOutput) override;

		void updateBinaryOutput(BinaryOutput &binaryOutput) override;

		void removeBinaryOutput(const uint32_t &deviceId) override;

		std::unique_ptr<BinaryOutput> getBinaryOutput(const uint32_t &deviceId) override;

		std::unique_ptr<BinaryOutput> getBinaryOutputByDeviceId(const uint32_t &deviceId) override;

		std::set<uint8_t> getBinaryOutputAddresses() override;

		std::map<uint8_t, uint8_t> getBinaryOutputCountMap() override;

		///// DALI API

		uint32_t insertDali(Dali &dali) override;

		void removeDali(const uint32_t &deviceId) override;

		std::unique_ptr<Dali> getDali(const uint32_t &id) override;

		std::unique_ptr<Dali> getDaliByDeviceId(const uint32_t &deviceId) override;

		std::set<uint8_t> getDaliAddresses() override;

		///// Devices API

		uint32_t insertDevice(Device &device) override;

		void updateDevice(Device &device) override;

		void removeDevice(const uint32_t &id) override;

		std::vector<Device> getDevices() override;

		std::unique_ptr<Device> getDevice(const uint32_t &id) override;

		std::unique_ptr<Device> getDevice(const uint8_t &address) override;

		bool deviceImplementsPeripheral(const uint32_t &id, int16_t peripheral) override;

		std::set<uint8_t> getDeviceAddresses() override;

		uint16_t getDeviceHwpid(const uint8_t &address) override;

		uint32_t  getDeviceMid(const uint8_t &address) override;

		std::string getDeviceMetadata(const uint8_t &address) override;

		rapidjson::Document getDeviceMetadataDoc(const uint8_t &address) override;

		void setDeviceMetadata(const uint8_t &address, const std::string &metadata) override;

		std::vector<DeviceTuple> getDevicesWithProductInfo(std::vector<uint8_t> requestedDevices) override;

		///// Device sensors API

		void insertDeviceSensor(DeviceSensor &deviceSensor) override;

		void updateDeviceSensor(DeviceSensor &deviceSensor) override;

		void removeDeviceSensors(const uint8_t &address) override;

		std::unique_ptr<DeviceSensor> getDeviceSensor(const uint32_t &id) override;

		std::unique_ptr<DeviceSensor> getDeviceSensor(const uint8_t &address, const uint32_t &sensorId, const uint8_t &index) override;

		std::unique_ptr<DeviceSensor> getDeviceSensorByGlobalIndex(const uint8_t &address, const uint8_t &index) override;

		std::unique_ptr<DeviceSensor> getDeviceSensorByTypeIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) override;

		std::map<uint8_t, std::vector<std::tuple<DeviceSensor, Sensor>>> getDeviceSensorMap() override;

		void setDeviceSensorMetadata(DeviceSensor &deviceSensor, json &metadata, std::shared_ptr<std::string> updated) override;

		void setDeviceSensorMetadata(const uint8_t &address, const uint8_t &index, json &metadata, std::shared_ptr<std::string> updated) override;

		void setDeviceSensorMetadata(const uint8_t &address, const uint8_t &type, const uint8_t &index, nlohmann::json &metadata, std::shared_ptr<std::string> updated) override;

		void setDeviceSensorValue(DeviceSensor &deviceSensor, double &value, std::shared_ptr<std::string> timestamp) override;

		void setDeviceSensorValue(const uint8_t &address, const uint8_t &index, double &value, std::shared_ptr<std::string> timestamp) override;

		// FRC
		void setDeviceSensorValue(const uint8_t &address, const uint8_t &type, const uint8_t &index, double &value, std::shared_ptr<std::string> timestamp) override;

		void updateDeviceSensorValues(const std::map<uint8_t, std::vector<sensor::item::Sensor>> &devices) override;

		void updateDeviceSensorValues(const uint8_t &address, const std::string &sensors) override;

		void updateDeviceSensorValues(const uint8_t &type, const uint8_t &index, const std::set<uint8_t> &selectedNodes, const std::string &sensors) override;

		uint8_t getDeviceSensorGlobalIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) override;

		///// Drivers API

		uint32_t insertDriver(Driver &driver) override;

		void updateDriver(Driver &driver) override;

		void removeDriver(const uint32_t &id) override;

		std::unique_ptr<Driver> getDriver(const int16_t &peripheral, const double &version) override;

		std::vector<Driver> getDriversByProduct(const uint32_t &productId) override;

		std::vector<uint32_t> getDriverIdsByProduct(const uint32_t &productId) override;

		std::vector<Driver> getLatestDrivers() override;

		///// Light API

		uint32_t insertLight(Light &light) override;

		void updateLight(Light &light) override;

		void removeLight(const uint32_t &deviceId) override;

		std::unique_ptr<Light> getLight(const uint32_t &id) override;

		std::unique_ptr<Light> getLightByDeviceId(const uint32_t &deviceId) override;

		std::set<uint8_t> getLightAddresses() override;

		std::map<uint8_t, uint8_t> getLightCountMap() override;

		///// Products API

		uint32_t insertProduct(Product &product) override;

		void updateProduct(Product &product) override;

		void removeProduct(const uint32_t &id) override;

		std::unique_ptr<Product> getProduct(const uint32_t &id) override;

		std::unique_ptr<Product> getProduct(const uint16_t &hwpid, const uint16_t &hwpidVer, const uint16_t &osBuild, const uint16_t &dpa) override;

		uint32_t getCoordinatorProductId() override;

		std::vector<uint8_t> getProductDeviceAddresses(const uint32_t &productId) override;

		std::string getProductCustomDriver(const uint32_t &productId) override;

		///// Product drivers API

		void insertProductDriver(ProductDriver &productDriver) override;

		void removeProductDriver(const uint32_t &productId, const uint32_t &driverId) override;

		std::set<uint32_t> getProductDriversIds(const uint32_t &productId);

		std::map<uint32_t, std::set<uint32_t>> getProductsDriversIdMap() override;

		///// Sensors API

		uint32_t insertSensor(Sensor &sensor) override;

		void updateSensor(Sensor &sensor) override;

		void removeSensor(const uint32_t &id) override;

		std::unique_ptr<Sensor> getSensor(const uint8_t &type, const std::string &name) override;

		std::map<uint8_t, Sensor> getSensorsImplementedByDeviceMap(const uint8_t &address) override;

		///// Other API

		std::map<uint16_t, std::set<uint8_t>> getHwpidAddrsMapImplementingSensor(const uint8_t &type) override;

		SensorDataSelectMap getSensorDataSelectMap() override;

		/**
		 * Checks if metadata should be added to messages
		 * @return true if metadata should be added to messages, false otherwise
		 */
		bool addMetadataToMessage() override;

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

		/**
		 * Performs product package enumeration
		 */
		void productPackageEnumeration();

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
		 * Performs dali standard enumeration
		 * @param deviceId Device ID
		 * @param address Device address
		 */
		void daliEnumeration(const uint32_t &deviceId);

		/**
		 * Performs light standard enumeration
		 * @param deviceId Device ID
		 * @param address device address
		 */
		void lightEnumeration(const uint32_t &deviceId, const uint8_t &address);

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
		/// Path to database file
		std::string m_dbPath;
		/// Path to daemon js wrapper
		std::string m_wrapperPath;
		/// Database accessor
		std::shared_ptr<Storage> m_db = nullptr;
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
		bool m_metadataTomessages = false;
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
