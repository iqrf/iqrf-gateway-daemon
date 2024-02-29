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
#include <condition_variable>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
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
#include <sqlite_orm/sqlite_orm.h>

#include "Storage.h"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

using namespace iqrf::db;
using namespace sqlite_orm;
using json = nlohmann::json;

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
		 * Resets IQRF Database
		 */
		void resetDatabase() override;

		///// Transactions

		void beginTransaction() override;

		void finishTransaction() override;

		void cancelTransaction() override;

		///// Binary outputs

		std::unique_ptr<BinaryOutput> getBinaryOutput(const uint32_t &id) override;

		std::unique_ptr<BinaryOutput> getBinaryOutputByDevice(const uint32_t &deviceId) override;

		uint32_t insertBinaryOutput(BinaryOutput &binaryOutput) override;

		void updateBinaryOutput(BinaryOutput &binaryOutput) override;

		void removeBinaryOutput(const uint32_t &deviceId) override;

		std::map<uint8_t, uint8_t> getBinaryOutputCountMap() override;

		///// DALIs

		std::unique_ptr<Dali> getDaliByDevice(const uint32_t &deviceId) override;

		uint32_t insertDali(Dali &dali) override;

		void removeDali(const uint32_t &deviceId) override;

		std::set<uint8_t> getDaliDeviceAddresses() override;

		///// Devices

		std::vector<Device> getAllDevices() override;

		std::unique_ptr<Device> getDevice(const uint32_t &id) override;

		std::unique_ptr<Device> getDevice(const uint8_t &address) override;

		uint32_t insertDevice(Device &device) override;

		void updateDevice(Device &device) override;

		void removeDevice(const uint32_t &id) override;

		bool deviceImplementsPeripheral(const uint32_t &id, int16_t peripheral) override;

		uint16_t getDeviceHwpid(const uint8_t &address) override;

		uint32_t  getDeviceMid(const uint8_t &address) override;

		std::string getDeviceMetadata(const uint8_t &address) override;

		rapidjson::Document getDeviceMetadataDoc(const uint8_t &address) override;

		void setDeviceMetadata(const uint8_t &address, const std::string &metadata) override;

		std::vector<DeviceTuple> getDevicesWithProductInfo(std::vector<uint8_t> requestedDevices) override;

		///// Device sensors

		std::unique_ptr<DeviceSensor> getDeviceSensor(const uint32_t &id) override;

		std::unique_ptr<DeviceSensor> getDeviceSensor(const uint8_t &address, const uint32_t &sensorId, const uint8_t &index) override;

		std::unique_ptr<DeviceSensor> getDeviceSensorByGlobalIndex(const uint8_t &address, const uint8_t &index) override;

		std::unique_ptr<DeviceSensor> getDeviceSensorByTypeIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) override;

		void insertDeviceSensor(DeviceSensor &deviceSensor) override;

		void updateDeviceSensor(DeviceSensor &deviceSensor) override;

		void removeDeviceSensors(const uint8_t &address) override;

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

		///// Drivers

		std::unique_ptr<Driver> getDriver(const int16_t &peripheral, const double &version) override;

		std::vector<Driver> getDriversByProduct(const uint32_t &productId) override;

		std::vector<uint32_t> getDriverIdsByProduct(const uint32_t &productId) override;

		std::vector<Driver> getLatestDrivers() override;

		uint32_t insertDriver(Driver &driver) override;

		///// Lights

		std::unique_ptr<Light> getLight(const uint32_t &id) override;

		std::unique_ptr<Light> getLightByDevice(const uint32_t &deviceId) override;

		uint32_t insertLight(Light &light) override;

		void updateLight(Light &light) override;

		void removeLight(const uint32_t &deviceId) override;

		std::map<uint8_t, uint8_t> getLightCountMap() override;

		///// Products

		std::unique_ptr<Product> getProduct(const uint32_t &id) override;

		std::unique_ptr<Product> getProduct(const uint16_t &hwpid, const uint16_t &hwpidVer, const uint16_t &osBuild, const uint16_t &dpa) override;

		uint32_t insertProduct(Product &product) override;

		uint32_t getCoordinatorProductId() override;

		std::vector<uint8_t> getProductDeviceAddresses(const uint32_t &productId) override;

		std::string getProductCustomDriver(const uint32_t &productId) override;

		///// Product drivers

		void insertProductDriver(ProductDriver &productDriver) override;

		std::map<uint32_t, std::set<uint32_t>> getProductsDriversIdMap() override;

		///// Sensors

		std::unique_ptr<Sensor> getSensor(const uint8_t &type, const std::string &name) override;

		uint32_t insertSensor(Sensor &sensor) override;

		std::map<uint8_t, Sensor> getSensorsImplementedByDeviceMap(const uint8_t &address) override;

		///// Other

		std::map<uint16_t, std::set<uint8_t>> getHwpidAddrsMapImplementingSensor(const uint8_t &type) override;

		SensorDataSelectMap getSensorDataSelectMap() override;

		bool addMetadataToMessage() override;

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

	private:
		/**
		 * Initializes database
		 */
		void initializeDatabase();

		/// Launch service
		shape::ILaunchService *m_launchService = nullptr;
		/// Component instance name
		std::string m_instance;
		/// Path to database file
		std::string m_dbPath;
		/// Include device metadata in responses
		bool m_metadataTomessages = false;
		/// Database accessor
		std::shared_ptr<Storage> m_db = nullptr;
	};
}
