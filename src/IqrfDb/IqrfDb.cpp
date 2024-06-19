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

#include "IqrfDb.h"

#include "iqrf__IqrfDb.hxx"

TRC_INIT_MODULE(iqrf::IqrfDb);

using json = nlohmann::json;
using namespace sqlite_orm;

namespace iqrf {

	IqrfDb::IqrfDb() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	IqrfDb::~IqrfDb() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// Interface implementation /////

	void IqrfDb::resetDatabase() {
		TRC_FUNCTION_ENTER("");
		std::ifstream dbFile(m_dbPath);
		if (dbFile.is_open()) { // db file exists
			if (std::remove(m_dbPath.c_str()) != 0) {
				THROW_EXC_TRC_WAR(std::logic_error, "Failed to remove db file: " << strerror(errno));
			};
		}
		initializeDatabase();
		TRC_FUNCTION_LEAVE("");
	}

	bool IqrfDb::isRunning() {
		return m_exclusiveAccess != nullptr && m_enumRun;
	}

	void IqrfDb::enumerate(IIqrfDb::EnumParams &parameters) {
		TRC_FUNCTION_ENTER("");
		m_enumRun = true;
		m_enumRepeat = true;
		this->startEnumerationThread(parameters);
		{
			std::unique_lock<std::mutex> lock(m_enumMutex);
			m_enumCv.notify_all();
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::sendEnumerationResponse(EnumerationProgress progress) {
		try {
			for (auto &handler : m_enumHandlers) {
				handler.second(progress);
			}
		} catch (const std::exception &e) {
			CATCH_EXC_TRC_WAR(std::exception, e, "Invalid enumeration handler.");
		}
	}

	void IqrfDb::reloadDrivers() {
      TRC_FUNCTION_ENTER("");

      if (m_renderService != nullptr) {
        m_renderService->clearContexts();
      }
      loadCoordinatorDrivers();
      loadProductDrivers();

      TRC_FUNCTION_LEAVE("");
    }

	void IqrfDb::reloadCoordinatorDrivers() {
		TRC_FUNCTION_ENTER("");
		loadCoordinatorDrivers();
		TRC_FUNCTION_LEAVE("");
	}

	///// BINARY OUTPUT API

	uint32_t IqrfDb::inseryBinaryOutput(BinaryOutput &binaryOutput) {
		return m_db->insert<BinaryOutput>(binaryOutput);
	}

	void IqrfDb::updateBinaryOutput(BinaryOutput &binaryOutput) {
		m_db->update<BinaryOutput>(binaryOutput);
	}

	void IqrfDb::removeBinaryOutput(const uint32_t &deviceId) {
		m_db->remove<BinaryOutput>(
			where(c(&BinaryOutput::getDeviceId) == deviceId)
		);
	}

	std::unique_ptr<BinaryOutput> IqrfDb::getBinaryOutput(const uint32_t &id) {
		return m_db->get_pointer<BinaryOutput>(id);
	}

	std::unique_ptr<BinaryOutput> IqrfDb::getBinaryOutputByDeviceId(const uint32_t &deviceId) {
		auto records = m_db->get_all<BinaryOutput>(
			where(c(&BinaryOutput::getDeviceId) == deviceId)
		);
		if (records.size() == 0) {
			return nullptr;
		}
		return std::make_unique<BinaryOutput>(records[0]);
	}

	std::set<uint8_t> IqrfDb::getBinaryOutputAddresses() {
		auto records = m_db->select(&Device::getAddress,
			inner_join<BinaryOutput>(on(c(&BinaryOutput::getDeviceId) == &Device::getId))
		);
		std::set<uint8_t> addresses;
		for (auto &addr : records) {
			addresses.insert(addr);
		}
		return addresses;
	}

	std::map<uint8_t, uint8_t> IqrfDb::getBinaryOutputCountMap() {
		auto records = m_db->select(
			columns(&Device::getAddress, &BinaryOutput::getCount),
			inner_join<BinaryOutput>(on(c(&BinaryOutput::getDeviceId) == &Device::getId))
		);
		std::map<uint8_t, uint8_t> boCountMap;
		for (auto &record : records) {
			boCountMap.insert(std::make_pair(std::get<0>(record), std::get<1>(record)));
		}
		return boCountMap;
	}

	///// DALI API

	uint32_t IqrfDb::insertDali(Dali &dali) {
		return m_db->insert<Dali>(dali);
	}

	void IqrfDb::removeDali(const uint32_t &deviceId) {
		m_db->remove<Dali>(
			where(c(&Dali::getDeviceId) == deviceId)
		);
	}

	std::unique_ptr<Dali> IqrfDb::getDali(const uint32_t &id) {
		return m_db->get_pointer<Dali>(id);
	}

	std::unique_ptr<Dali> IqrfDb::getDaliByDeviceId(const uint32_t &deviceId) {
		auto records = m_db->get_all<Dali>(
			where(c(&Dali::getDeviceId) == deviceId)
		);
		if (records.size() == 0) {
			return nullptr;
		}
		return std::make_unique<Dali>(records[0]);
	}

	std::set<uint8_t> IqrfDb::getDaliAddresses() {
		auto records = m_db->select(&Device::getAddress,
			inner_join<Dali>(on(c(&Dali::getDeviceId) == &Device::getId))
		);
		std::set<uint8_t> addresses;
		for (auto &addr : records) {
			addresses.insert(addr);
		}
		return addresses;
	}

	///// Devices API

	uint32_t IqrfDb::insertDevice(Device &device) {
		return m_db->insert<Device>(device);
	}

	void IqrfDb::updateDevice(Device &device) {
		m_db->update<Device>(device);
	}

	void IqrfDb::removeDevice(const uint32_t &id) {
		m_db->remove<Device>(id);
	}

	std::vector<Device> IqrfDb::getDevices() {
		return m_db->get_all<Device>();
	}

	std::unique_ptr<Device> IqrfDb::getDevice(const uint32_t &id) {
		return m_db->get_pointer<Device>(id);
	}

	std::unique_ptr<Device> IqrfDb::getDevice(const uint8_t &address) {
		auto records = m_db->get_all<Device>(
			where(c(&Device::getAddress) == address)
		);
		if (records.size() == 0) {
			return nullptr;
		}
		return std::make_unique<Device>(records[0]);
	}

	bool IqrfDb::deviceImplementsPeripheral(const uint32_t &id, int16_t peripheral) {
		auto records = m_db->select(&Driver::getId,
			inner_join<ProductDriver>(on(c(&ProductDriver::getDriverId) == &Driver::getId)),
			inner_join<Device>(on(c(&Device::getProductId) == &ProductDriver::getProductId)),
			where(
				c(&Device::getId) == id
				and c(&Driver::getPeripheralNumber) == peripheral
			)
		);
		return records.size() > 0;
	}

	std::set<uint8_t> IqrfDb::getDeviceAddresses() {
		auto records = m_db->get_all<Device>();
		std::set<uint8_t> addresses;
		for (auto &record : records) {
			addresses.insert(record.getAddress());
		}
		return addresses;
	}

	uint16_t IqrfDb::getDeviceHwpid(const uint8_t &address) {
		auto hwpids = m_db->select(&Product::getHwpid,
			inner_join<Product>(on(c(&Product::getId) == &Device::getProductId)),
			where(c(&Device::getAddress) == address)
		);
		if (hwpids.size() == 0) {
			throw std::logic_error("Device at address " + std::to_string(address) + " does not exist.");
		}
		return hwpids[0];
	}

	uint32_t IqrfDb::getDeviceMid(const uint8_t &address) {
		auto devicePtr = this->getDevice(address);
		if (devicePtr == nullptr) {
			throw std::logic_error("Device at address " + std::to_string(address) + " does not exist.");
		}
		return devicePtr->getMid();
	}

	std::string IqrfDb::getDeviceMetadata(const uint8_t &address) {
		auto device = this->getDevice(address);
		if (device == nullptr) {
			throw std::logic_error("Device at address " + std::to_string(address) + " does not exist.");
		}
		auto name = device->getName();
		auto location = device->getLocation();
		auto metadata = device->getMetadata();
		json j = {
			{"name", name == nullptr ? nullptr : *name.get()},
			{"location", location == nullptr ? nullptr : *location.get()},
			{"other", metadata == nullptr ? nullptr : *metadata.get()},
		};
		return j.dump();
	}

	rapidjson::Document IqrfDb::getDeviceMetadataDoc(const uint8_t &address) {
		std::string metadata = this->getDeviceMetadata(address);
		rapidjson::Document doc;
		doc.Parse(metadata.c_str());
		if (doc.HasParseError()) {
			THROW_EXC_TRC_WAR(std::logic_error, "Invalid json syntax in metadata: " << doc.GetParseError() << ", " << doc.GetErrorOffset());
		}
		return doc;
	}

	void IqrfDb::setDeviceMetadata(const uint8_t &address, const std::string &metadata) {
		auto devicePtr = this->getDevice(address);
		if (devicePtr == nullptr) {
			throw std::logic_error("Device at address " + std::to_string(address) + " does not exist.");
		}
		Device d = *devicePtr.get();
		json j = json::parse(metadata);
		if (j.find("name") != j.end()) {
			if (j["name"].is_null()) {
				d.setName(nullptr);
			} else {
				std::string name = j["name"].get<std::string>();
				d.setName(std::make_shared<std::string>(name));
			}
		}
		if (j.find("location") != j.end()) {
			if (j["location"].is_null()) {
				d.setLocation(nullptr);
			} else {
				std::string location = j["location"].get<std::string>();
				d.setLocation(std::make_shared<std::string>(location));
			}
		}
		if (j.find("other") != j.end()) {
			if (j["other"].is_null() || j["other"].empty()) {
				d.setMetadata(nullptr);
			} else {
				std::string other = j["other"].dump();
				d.setMetadata(std::make_shared<std::string>(other));
			}
		}
		m_db->update(d);
	}

	std::vector<DeviceTuple> IqrfDb::getDevicesWithProductInfo(std::vector<uint8_t> requestedDevices) {
		std::vector<DeviceTuple> devices;
		std::vector<Device> dbDevices;
		if (requestedDevices.size() == 0) {
			dbDevices = m_db->get_all<Device>();
		} else {
			dbDevices = m_db->get_all<Device>(where(in(&Device::getAddress, requestedDevices)));
		}
		for (auto &device : dbDevices) {
			uint32_t productId = device.getProductId();
			Product product = m_db->get<Product>(productId);
			devices.push_back(std::make_tuple(device, product.getHwpid(), product.getHwpidVersion(), product.getOsBuild(), product.getOsVersion(), product.getDpaVersion()));
		}
		return devices;
	}

	///// Device sensors API

	void IqrfDb::insertDeviceSensor(DeviceSensor &deviceSensor) {
		m_db->replace<DeviceSensor>(deviceSensor);
	}

	void IqrfDb::updateDeviceSensor(DeviceSensor &deviceSensor) {
		m_db->update<DeviceSensor>(deviceSensor);
	}

	void IqrfDb::removeDeviceSensors(const uint8_t &address) {
		m_db->remove_all<DeviceSensor>(where(c(&DeviceSensor::getAddress) == address));
	}

	std::unique_ptr<DeviceSensor> IqrfDb::getDeviceSensor(const uint32_t &id) {
		return m_db->get_pointer<DeviceSensor>(id);
	}

	std::unique_ptr<DeviceSensor> IqrfDb::getDeviceSensor(const uint8_t &address, const uint32_t &sensorId, const uint8_t &index) {
		auto records = m_db->get_all<DeviceSensor>(
			where(
				c(&DeviceSensor::getAddress) == address
				and c(&DeviceSensor::getSensorId) == sensorId
				and c(&DeviceSensor::getGlobalIndex) == index
			)
		);
		if (records.size() == 0) {
			return nullptr;
		}
		return std::make_unique<DeviceSensor>(records[0]);
	}

	std::unique_ptr<DeviceSensor> IqrfDb::getDeviceSensorByGlobalIndex(const uint8_t &address, const uint8_t &index) {
		auto records = m_db->get_all<DeviceSensor>(
			where(
				c(&DeviceSensor::getAddress) == address
				and c(&DeviceSensor::getGlobalIndex) == index
			)
		);
		if (records.size() == 0) {
			throw std::logic_error("Device at address " + std::to_string(address) + " does not implement sensor at index " + std::to_string(index));
		}
		return std::make_unique<DeviceSensor>(records[0]);
	}

	std::unique_ptr<DeviceSensor> IqrfDb::getDeviceSensorByTypeIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) {
		auto records = m_db->get_all<DeviceSensor>(
			where(
				c(&DeviceSensor::getAddress) == address
				and c(&DeviceSensor::getType) == type
				and c(&DeviceSensor::getTypeIndex) == index
			)
		);
		if (records.size() == 0) {
			throw std::logic_error("Device at address " + std::to_string(address) + " does not implement sensor type " + std::to_string(type) + " index " + std::to_string(index));
		}
		return std::make_unique<DeviceSensor>(records[0]);
	}

	std::map<uint8_t, std::vector<std::tuple<DeviceSensor, Sensor>>> IqrfDb::getDeviceSensorMap() {
		std::map<uint8_t, std::vector<std::tuple<DeviceSensor, Sensor>>> sensors;
		auto deviceSensors = m_db->get_all<DeviceSensor>();
		for (auto &item : deviceSensors) {
			auto device = m_db->get_all<Device>(where(c(&Device::getAddress) == item.getAddress()));
			if (device.size() == 0) {
				continue;
			}
			Sensor sensor = m_db->get<Sensor>(item.getSensorId());
			uint8_t address = device[0].getAddress();
			if (sensors.count(address) == 0) {
				std::vector<std::tuple<DeviceSensor, Sensor>> v{std::make_tuple(item, sensor)};
				sensors.insert(std::make_pair(address, v));
			} else {
				sensors[address].push_back(std::make_tuple(item, sensor));
			}
		}
		return sensors;
	}

	void IqrfDb::setDeviceSensorMetadata(DeviceSensor &deviceSensor, json &metadata, std::shared_ptr<std::string> updated) {
		std::shared_ptr<std::string> current = deviceSensor.getMetadata();
		if (current) {
			json j = json::parse(*current.get());
			if (j.count("datablock")) {
				metadata["datablock"] = j["datablock"];
			}
		}
		current = std::make_shared<std::string>(metadata.dump());
		if (metadata.count("datablock")) {
			deviceSensor.setUpdated(updated);
		}
		deviceSensor.setMetadata(current);
		this->updateDeviceSensor(deviceSensor);
	}

	void IqrfDb::setDeviceSensorMetadata(const uint8_t &address, const uint8_t &index, json &metadata, std::shared_ptr<std::string> updated) {
		auto deviceSensorPtr = this->getDeviceSensorByGlobalIndex(address, index);
		this->setDeviceSensorMetadata(*deviceSensorPtr.get(), metadata, updated);
	}

	void IqrfDb::setDeviceSensorMetadata(const uint8_t &address, const uint8_t &type, const uint8_t &index, nlohmann::json &metadata, std::shared_ptr<std::string> updated) {
		auto deviceSensorPtr = this->getDeviceSensorByTypeIndex(address, type, index);
		this->setDeviceSensorMetadata(*deviceSensorPtr.get(), metadata, updated);
	}

	void IqrfDb::setDeviceSensorValue(DeviceSensor &deviceSensor, double &value, std::shared_ptr<std::string> timestamp) {
		deviceSensor.setValue(std::make_shared<double>(value));
		deviceSensor.setUpdated(timestamp);
		this->updateDeviceSensor(deviceSensor);
	}

	void IqrfDb::setDeviceSensorValue(const uint8_t &address, const uint8_t &index, double &value, std::shared_ptr<std::string> timestamp) {
		auto deviceSensorPtr = this->getDeviceSensorByGlobalIndex(address, index);
		this->setDeviceSensorValue(*deviceSensorPtr.get(), value, timestamp);
	}

	// FRC
	void IqrfDb::setDeviceSensorValue(const uint8_t &address, const uint8_t &type, const uint8_t &index, double &value, std::shared_ptr<std::string> timestamp) {
		auto deviceSensorPtr = this->getDeviceSensorByTypeIndex(address, type, index);
		this->setDeviceSensorValue(*deviceSensorPtr.get(), value, timestamp);
	}

	void IqrfDb::updateDeviceSensorValues(const std::map<uint8_t, std::vector<sensor::item::Sensor>> &devices) {
		TRC_FUNCTION_ENTER("");
		std::shared_ptr<std::string> timestamp = IqrfDbAux::getCurrentTimestamp();
		for (auto &device : devices) {
			const uint8_t addr = device.first;
			auto dbDevice = this->getDevice(addr);
			if (dbDevice == nullptr) {
				continue;
			}
			for (auto &sensor : device.second) {
				if (!sensor.isValueSet()) {
					continue;
				}
				try {
					if (sensor.getType() == 192) {
						auto &v = sensor.hasBreakdown() ? sensor.getBreakdownValueArray() : sensor.getValueArray();
						json block = {{"datablock", json(v)}};
						this->setDeviceSensorMetadata(addr, sensor.getIdx(), block, timestamp);
					} else {
						double val;
						if (sensor.hasBreakdown()) {
							val = sensor.getBreakdownValue();
						} else {
							val = sensor.getValue();
						}
						this->setDeviceSensorValue(addr, sensor.getIdx(), val, timestamp);
					}
				} catch (const std::logic_error &e) {
					TRC_WARNING(e.what());
				}
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::updateDeviceSensorValues(const uint8_t &address, const std::string &sensors) {
		TRC_FUNCTION_ENTER("");
		json j = json::parse(sensors);
		std::shared_ptr<std::string> timestamp = IqrfDbAux::getCurrentTimestamp();
		for (uint8_t i = 0, n = j["sensors"].size(); i < n; ++i) {
			json item = j["sensors"][i];
			double val;
			try {
				if (item["value"].is_null()) {
					continue;
				}
				if (item["type"] == 192) {
					if (item["value"].is_null()) {
						continue;
					}
					json block = {{"datablock", item["value"]}};
					this->setDeviceSensorMetadata(address, i, block, timestamp);
					continue;
				}
				val = item["value"];
				if (item["type"] == 129 || item["type"] == 160) {
					if (!item["breakdown"][0]["value"].is_null()) {
						val = item["breakdown"][0]["value"];
					}
				}
				this->setDeviceSensorValue(address, i, val, timestamp);
			} catch (const std::logic_error &e) {
				TRC_WARNING(e.what());
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::updateDeviceSensorValues(const uint8_t &type, const uint8_t &index, const std::set<uint8_t> &selectedNodes, const std::string &sensors) {
		TRC_FUNCTION_ENTER("");
		json j = json::parse(sensors);
		std::shared_ptr<std::string> timestamp = IqrfDbAux::getCurrentTimestamp();
		if (selectedNodes.size() == 0) {
			for (uint8_t i = 0, n = j["sensors"].size(); i < n; ++i) {
				json item = j["sensors"][i];
				if (item["value"].is_null()) {
					continue;
				}
				if (type == 192) {
					json block = {{"datablock", item["value"]}};
					this->setDeviceSensorMetadata(i, type, index, block, timestamp);
					continue;
				}
				double val;
				try {
					val = item["value"];
					if (item["type"] == 129 || item["type"] == 160) {
						if (!item["breakdown"][0]["value"].is_null()) {
							val = item["breakdown"][0]["value"];
						}
					}
					this->setDeviceSensorValue(i, type, index, val, timestamp);
				} catch (const std::logic_error &e) {
					TRC_WARNING(e.what());
				}
			}
		} else {
			uint8_t i = 1;
			for (auto it = selectedNodes.begin(); it != selectedNodes.end(); ++it, ++i) {
				json item = j["sensors"][i];
				if (item["value"].is_null()) {
					continue;
				}
				if (type == 192) {
					json block = {{"datablock", item["value"]}};
					this->setDeviceSensorMetadata(i, type, index, block, timestamp);
					continue;
				}
				double val;
				try {
					val = item["value"];
					if (item["type"] == 129 || item["type"] == 160) {
						if (!item["breakdown"][0]["value"].is_null()) {
							val = item["breakdown"][0]["value"];
						}
					}
					this->setDeviceSensorValue(i, type, index, val, timestamp);
				} catch (const std::logic_error &e) {
					TRC_WARNING(e.what());
				}
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	uint8_t IqrfDb::getDeviceSensorGlobalIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) {
		auto records = m_db->get_all<DeviceSensor>(
			where(
				c(&DeviceSensor::getAddress) == address
				and c(&DeviceSensor::getType) == type
				and c(&DeviceSensor::getTypeIndex) == index
			)
		);
		if (records.size() == 0) {
			throw std::logic_error("Device at address " + std::to_string(address)
				+ " does not implement sensor of type " + std::to_string(type)
				+ " at index " + std::to_string(index)
			);
		}
		return records[0].getGlobalIndex();
	}

	///// Drivers API

	uint32_t IqrfDb::insertDriver(Driver &driver) {
		return m_db->insert<Driver>(driver);
	}

	void IqrfDb::updateDriver(Driver &driver) {
		m_db->update<Driver>(driver);
	}

	void IqrfDb::removeDriver(const uint32_t &id) {
		m_db->remove<Driver>(id);
	}

	std::unique_ptr<Driver> IqrfDb::getDriver(const int16_t &peripheral, const double &version) {
		auto records = m_db->get_all<Driver>(
			where(
				c(&Driver::getPeripheralNumber) == peripheral
				and c(&Driver::getVersion) == version
			)
		);
		if (records.size() == 0) {
			return nullptr;
		}
		return std::make_unique<Driver>(records[0]);
	}

	std::vector<Driver> IqrfDb::getDriversByProduct(const uint32_t &productId) {
		auto records = m_db->get_all<ProductDriver>(where(c(&ProductDriver::getProductId) == productId));
		std::vector<Driver> drivers;
		for (auto &pd : records) {
			drivers.push_back(m_db->get<Driver>(pd.getDriverId()));
		}
		return drivers;
	}

	std::vector<uint32_t> IqrfDb::getDriverIdsByProduct(const uint32_t &productId) {
		return m_db->select(
			&ProductDriver::getDriverId,
			where(c(&ProductDriver::getProductId) == productId)
		);
	}

	std::vector<Driver> IqrfDb::getLatestDrivers() {
		return m_db->get_all<Driver>(max(&Driver::getVersion), group_by(&Driver::getPeripheralNumber));
	}

	///// Light API

	uint32_t IqrfDb::insertLight(Light &light) {
		return m_db->insert<Light>(light);
	}

	void IqrfDb::updateLight(Light &light) {
		m_db->update<Light>(light);
	}

	void IqrfDb::removeLight(const uint32_t &deviceId) {
		m_db->remove<Light>(
			where(c(&Light::getDeviceId) == deviceId)
		);
	}

	std::unique_ptr<Light> IqrfDb::getLight(const uint32_t &id) {
		return m_db->get_pointer<Light>(id);
	}

	std::unique_ptr<Light> IqrfDb::getLightByDeviceId(const uint32_t &deviceId) {
		auto records = m_db->get_all<Light>(
			where(c(&Light::getDeviceId) == deviceId)
		);
		if (records.size() == 0) {
			return nullptr;
		}
		return std::make_unique<Light>(records[0]);
	}

	std::set<uint8_t> IqrfDb::getLightAddresses() {
		auto records = m_db->select(&Device::getAddress,
			inner_join<Dali>(on(c(&Dali::getDeviceId) == &Device::getId))
		);
		std::set<uint8_t> addresses;
		for (auto &addr : records) {
			addresses.insert(addr);
		}
		return addresses;
	}

	std::map<uint8_t, uint8_t> IqrfDb::getLightCountMap() {
		auto records = m_db->select(columns(&Device::getAddress, &Light::getCount),
			inner_join<Light>(on(c(&Light::getDeviceId) == &Device::getId))
		);
		std::map<uint8_t, uint8_t> lightCountMap;
		for (auto &record : records) {
			lightCountMap.insert(std::make_pair(std::get<0>(record), std::get<1>(record)));
		}
		return lightCountMap;
	}

	///// Products API

	uint32_t IqrfDb::insertProduct(Product &product) {
		return m_db->insert<Product>(product);
	}

	void IqrfDb::updateProduct(Product &product) {
		m_db->update<Product>(product);
	}

	void IqrfDb::removeProduct(const uint32_t &id) {
		m_db->remove<Product>(id);
	}

	std::unique_ptr<Product> IqrfDb::getProduct(const uint32_t &id) {
		return m_db->get_pointer<Product>(id);
	}

	std::unique_ptr<Product> IqrfDb::getProduct(const uint16_t &hwpid, const uint16_t &hwpidVer, const uint16_t &osBuild, const uint16_t &dpa) {
		auto records = m_db->get_all<Product>(
			where(
				c(&Product::getHwpid) == hwpid
				and c(&Product::getHwpidVersion) == hwpidVer
				and c(&Product::getOsBuild) == osBuild
				and c(&Product::getDpaVersion) == dpa
			)
		);
		if (records.size() == 0) {
			return nullptr;
		}
		return std::make_unique<Product>(records[0]);
	}

	uint32_t IqrfDb::getCoordinatorProductId() {
		auto productId = m_db->select(&Product::getId,
			inner_join<Device>(on(c(&Device::getProductId) == &Product::getId)),
			where(c(&Device::getAddress) == 0)
		);
		if (productId.size() == 0) {
			return 0;
		}
		return productId[0];
	}

	std::vector<uint8_t> IqrfDb::getProductDeviceAddresses(const uint32_t &productId) {
		return m_db->select(&Device::getAddress, where(c(&Device::getProductId) == productId));
	}

	std::string IqrfDb::getProductCustomDriver(const uint32_t &productId) {
		auto entity = m_db->get<Product>(productId);
		std::shared_ptr<std::string> customDriver = entity.getCustomDriver();
		if (customDriver == nullptr) {
			return "";
		}
		return *customDriver.get();
	}

	///// Product drivers API

	void IqrfDb::insertProductDriver(ProductDriver &productDriver) {
		m_db->replace(productDriver);
	}
	
	void IqrfDb::removeProductDriver(const uint32_t &productId, const uint32_t &driverId) {
		m_db->remove_all<ProductDriver>(
			where(
				c(&ProductDriver::getProductId) == productId and
				c(&ProductDriver::getDriverId) == driverId
			)
		);
	}

	std::set<uint32_t> IqrfDb::getProductDriversIds(const uint32_t &productId) {
		auto records = m_db->get_all<ProductDriver>(where(c(&ProductDriver::getProductId) == productId));
		std::set<uint32_t> driversIds;
		for (auto &pd : records) {
			driversIds.insert(pd.getDriverId());
		}
		return driversIds;
	}

	std::map<uint32_t, std::set<uint32_t>> IqrfDb::getProductsDriversIdMap() {
		auto records = m_db->select(columns(&Product::getId, &Driver::getId),
			inner_join<ProductDriver>(on(c(&ProductDriver::getDriverId) == &Driver::getId)),
			inner_join<Product>(on(c(&Product::getId) == &ProductDriver::getProductId))
		);

		std::map<uint32_t, std::set<uint32_t>> productDrivers;

		for (auto &record : records) {
			uint32_t productId = std::get<0>(record);
			uint32_t driverId = std::get<1>(record);
			if (productDrivers.count(productId) == 0) {
				productDrivers.insert(std::make_pair(productId, std::set<uint32_t>{driverId}));
			} else {
				productDrivers[productId].insert(driverId);
			}
		}
		return productDrivers;
	}

	///// Sensors API
	
	uint32_t IqrfDb::insertSensor(Sensor &sensor) {
		return m_db->insert<Sensor>(sensor);
	}

	void IqrfDb::updateSensor(Sensor &sensor) {
		m_db->update<Sensor>(sensor);
	}

	void IqrfDb::removeSensor(const uint32_t &id) {
		m_db->remove<Sensor>(id);
	}

	std::unique_ptr<Sensor> IqrfDb::getSensor(const uint8_t &type, const std::string &name) {
		auto entities = m_db->get_all<Sensor>(
			where(
				c(&Sensor::getType) == type
				and c(&Sensor::getName) == name
			)
		);
		if (entities.size() == 0) {
			return nullptr;
		}
		return std::make_unique<Sensor>(entities[0]);
	}

	std::map<uint8_t, Sensor> IqrfDb::getSensorsImplementedByDeviceMap(const uint8_t &address) {
		auto deviceSensors = m_db->get_all<DeviceSensor>(where(c(&DeviceSensor::getAddress) == address));
		std::map<uint8_t, Sensor> sensors;
		for (auto ds : deviceSensors) {
			auto sensor = m_db->get<Sensor>(ds.getSensorId());
			sensors.insert(std::make_pair(ds.getGlobalIndex(), sensor));
		}
		return sensors;
	}

	///// Other API

	std::map<uint16_t, std::set<uint8_t>> IqrfDb::getHwpidAddrsMapImplementingSensor(const uint8_t &type) {
		auto rows = m_db->select(columns(&Product::getHwpid, &Device::getAddress),
			inner_join<Product>(on(c(&Product::getId) == &Device::getProductId)),
			inner_join<DeviceSensor>(on(c(&DeviceSensor::getAddress) == &Device::getAddress)),
			where(c(&DeviceSensor::getType) == type),
			group_by(&Device::getAddress)
		);
		std::map<uint16_t, std::set<uint8_t>> map;
		for (const auto &v : rows) {
			uint16_t hwpid = std::get<0>(v);
			uint8_t address = std::get<1>(v);
			if (map.find(hwpid) != map.end()) {
				map[hwpid].insert(address);
			} else {
				map.insert(std::make_pair(hwpid, std::set<uint8_t>({address})));
			}
		}
		return map;
	}

	SensorDataSelectMap IqrfDb::getSensorDataSelectMap() {
		SensorDataSelectMap map;
		auto deviceSensors = m_db->get_all<DeviceSensor>(
			multi_order_by(
				order_by(&DeviceSensor::getAddress).asc(),
				order_by(&DeviceSensor::getGlobalIndex).asc()
			)
		);
		uint8_t lastAddr = 0;
		for (auto &ds : deviceSensors) {
			uint8_t address = ds.getAddress();
			uint8_t type = ds.getType();
			if (lastAddr != address) {
				lastAddr = address;
			}
			map[type].emplace_back(std::make_tuple(address, ds.getTypeIndex()));
		}
		return map;
	}

	bool IqrfDb::addMetadataToMessage() {
		return m_metadataTomessages;
	}

	void IqrfDb::registerEnumerationHandler(const std::string &clientId, EnumerationHandler handler) {
		std::lock_guard<std::mutex> lock(m_enumMutex);
		m_enumHandlers.insert(std::make_pair(clientId, handler));
	}

	void IqrfDb::unregisterEnumerationHandler(const std::string &clientId) {
		std::lock_guard<std::mutex> lock(m_enumMutex);
		m_enumHandlers.erase(clientId);
	}

	///// Private methods /////

	void IqrfDb::initializeDatabase() {
		m_db = std::make_shared<Storage>(initializeDb(m_dbPath));
		auto res = m_db->sync_schema();
	}

	void IqrfDb::startEnumerationThread(IIqrfDb::EnumParams &parameters) {
		TRC_FUNCTION_ENTER("");
		if (m_enumThreadRun) {
			m_params = parameters;
			return;
		}
		m_enumThreadRun = true;
		if (m_enumThread.joinable()) {
			m_enumThread.join();
		}
		m_enumThread = std::thread([&]() {
			runEnumeration(parameters);
		});
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::stopEnumerationThread() {
		TRC_FUNCTION_ENTER("");
		m_enumRun = false;
		m_enumCv.notify_all();
		if (m_enumThread.joinable()) {
			m_enumThread.join();
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::analyzeDpaMessage(const DpaMessage &message) {
		auto direction = message.MessageDirection();
		// not a response
		if (direction != DpaMessage::MessageType::kResponse) {
			return;
		}
		// async response
		if ((message.DpaPacket().DpaResponsePacket_t.ResponseCode & STATUS_ASYNC_RESPONSE)) {
			return;
		}
		// not coordinator device
		if (message.NodeAddress() != 0) {
			return;
		}
		// not coordinator peripheral
		if (message.PeripheralType() != 0) {
			return;
		}
		uint8_t pcmd = message.PeripheralCommand() & ~0x80;
		if (pcmd == CMD_COORDINATOR_BOND_NODE ||
			pcmd == CMD_COORDINATOR_CLEAR_ALL_BONDS ||
			pcmd == CMD_COORDINATOR_DISCOVERY ||
			pcmd == CMD_COORDINATOR_REMOVE_BOND ||
			pcmd == CMD_COORDINATOR_RESTORE ||
			pcmd == CMD_COORDINATOR_SET_MID ||
			pcmd == CMD_COORDINATOR_SMART_CONNECT
		) {
			TRC_INFORMATION("Automatic enumeration invoked by " << PAR(pcmd));
			m_enumRun = true;
			m_enumRepeat = true;
			m_enumCv.notify_all();
		}
 	}

	void IqrfDb::runEnumeration(IIqrfDb::EnumParams &parameters) {
		TRC_FUNCTION_ENTER("");

		m_params = parameters;

		while (m_enumThreadRun) {
			if (m_enumRun) {
				if (!m_dpaService->hasExclusiveAccess()) {
					waitForExclusiveAccess();
					TRC_INFORMATION("Running enumeration with: " << PAR(m_params.reenumerate) << PAR(m_params.standards));
					sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::Start));
					checkNetwork(m_params.reenumerate);
					sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::NetworkDone));
					resetExclusiveAccess();

					if (!m_enumThreadRun) {
						break;
					}

					waitForExclusiveAccess();
					sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::Devices));
					enumerateDevices();
					sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::DevicesDone));
					resetExclusiveAccess();

					if (!m_enumThreadRun) {
						break;
					}

					waitForExclusiveAccess();
					sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::Products));
					productPackageEnumeration();
					updateDatabaseProducts();
					loadProductDrivers();
					sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::ProductsDone));
					resetExclusiveAccess();

					if (!m_enumThreadRun) {
						break;
					}

					if (m_params.standards || m_params.reenumerate) {
						waitForExclusiveAccess();
						sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::Standards));
						standardEnumeration();
						sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::StandardsDone));
						resetExclusiveAccess();
					}
					m_enumRepeat = false;
					sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::Finish));
				} else {
					TRC_DEBUG("DPA has exclusive access.");
				}
				clearAuxBuffers();
			}

			// wait until next enumeration invocation
			std::unique_lock<std::mutex> lock(m_enumMutex);
			if (m_enumRepeat) {
				TRC_DEBUG("Enumeration failed, repeating enumeration.");
				m_enumCv.wait_for(lock, std::chrono::seconds(3));
			} else {
				TRC_DEBUG("Waiting until next enumeration is invoked.");
				m_enumCv.wait(lock);
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::checkNetwork(bool reenumerate) {
		TRC_FUNCTION_ENTER("");
		m_coordinatorParams = m_dpaService->getCoordinatorParameters();
		try {
			getBondedNodes();
			getDiscoveredNodes();
			getMids();
			getRoutingInformation();
			m_mids.insert(std::make_pair(0, m_coordinatorParams.mid));
		} catch (const std::exception &e) {
			THROW_EXC(std::logic_error, e.what());
		}

		auto dbDevices = this->getDevices();

		for (std::vector<Device>::iterator it = dbDevices.begin(); it != dbDevices.end(); ++it) {
			Device device = *it;
			uint8_t addr = device.getAddress();
			// remove devices that are not in network from db
			if (m_toEnumerate.find(addr) == m_toEnumerate.end()) {
				m_toDelete.insert(device.getId());
				continue;
			}

			if (!reenumerate) {
				uint32_t mid = device.getMid();
				if (m_mids[addr] == mid) {
					// do not enumerate this node
					m_toEnumerate.erase(addr);
				}
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::enumerateDevices() {
		TRC_FUNCTION_ENTER("");
		uint8_t enumerateDeviceCount = m_toEnumerate.size();
		// coordinator enumeration
		bool enumerateCoordinator = (enumerateDeviceCount > 0) && (*m_toEnumerate.begin() == 0);
		if (enumerateCoordinator) {
			--enumerateDeviceCount;
			coordinatorEnumeration();
			m_toEnumerate.erase(0);
		}
		// node enumeration
		if (enumerateDeviceCount > 1 && m_coordinatorParams.dpaVerWord >= 0x402) {
			frcEnumeration();
		} else {
			pollEnumeration();
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::getBondedNodes() {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		try {
			// Build DPA request
			DpaMessage bondedRequest;
			DpaMessage::DpaPacket_t bondedPacket;
			bondedPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			bondedPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
			bondedPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
			bondedPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			bondedRequest.DataToBuffer(bondedPacket.Buffer, sizeof(TDpaIFaceHeader));
			// Execute DPA request
			m_dpaService->executeDpaTransactionRepeat(bondedRequest, result, 1);
			DpaMessage bondedResponse = result->getResponse();
			// Process DPA response
			const unsigned char *pData = bondedResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
			for (uint8_t i = 1, n = MAX_ADDRESS; i <= n; i++) {
				if ((pData[i / 8] & (1 << (i % 8))) != 0) {
					m_toEnumerate.insert(i);
				}
			}
			m_toEnumerate.insert(0);
		} catch (const std::exception &e) {
			THROW_EXC(std::logic_error, e.what());
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::getDiscoveredNodes() {
		TRC_FUNCTION_ENTER("");
		if (m_toEnumerate.size() == 0) {
			return;
		}
		std::unique_ptr<IDpaTransactionResult2> result;
		try {
			// Build DPA request
			DpaMessage discoveredRequest;
			DpaMessage::DpaPacket_t discoveredPacket;
			discoveredPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			discoveredPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
			discoveredPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_DISCOVERED_DEVICES;
			discoveredPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			discoveredRequest.DataToBuffer(discoveredPacket.Buffer, sizeof(TDpaIFaceHeader));
			// Execute DPA request
			m_dpaService->executeDpaTransactionRepeat(discoveredRequest, result, 1);
			DpaMessage discoveredResponse = result->getResponse();
			// Process DPA responses
			const unsigned char *pData = discoveredResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
			for (uint8_t addr : m_toEnumerate) {
				if ((pData[addr / 8] & (1 << (addr % 8))) != 0) {
					m_discovered.insert(addr);
				}
			}
		} catch (const std::exception &e) {
			THROW_EXC(std::logic_error, e.what());
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::getMids() {
		TRC_FUNCTION_ENTER("");
		if (m_toEnumerate.size() == 0) {
			return;
		}
		// Prepare request parameters
		const uint8_t maxDataLen = 54;
		const uint16_t startAddr = 0x4000;
		const uint16_t totalData = (*m_toEnumerate.rbegin() + 1) * 8;
		const uint8_t requestCount = totalData / maxDataLen;
		const uint8_t remainder = totalData % maxDataLen;

		std::unique_ptr<IDpaTransactionResult2> result;
		try {
			// Build DPA request
			DpaMessage eeepromReadRequest;
			DpaMessage::DpaPacket_t eeepromReadPacket;
			eeepromReadPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			eeepromReadPacket.DpaRequestPacket_t.PNUM = PNUM_EEEPROM;
			eeepromReadPacket.DpaRequestPacket_t.PCMD = CMD_EEEPROM_XREAD;
			eeepromReadPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			// Collect MIDS from EEEPROM
			std::vector<uint8_t> eeepromData;
			for (uint8_t i = 0; i < requestCount + 1; i++) {
				uint8_t length = (uint8_t)(i < requestCount ? maxDataLen : remainder);
				if (length == 0) {
					continue;
				}
				uint16_t address = (uint16_t)(startAddr + i * maxDataLen);
				eeepromReadPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.Address = address;
				eeepromReadPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.ReadWrite.Read.Length = length;
				eeepromReadRequest.DataToBuffer(eeepromReadPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(uint16_t) + sizeof(uint8_t));
				// Execute DPA request
				m_dpaService->executeDpaTransactionRepeat(eeepromReadRequest, result, 1);
				DpaMessage eeepromResponse = result->getResponse();
				// Store EEEPROM data
				const unsigned char *pData = eeepromResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
				eeepromData.insert(eeepromData.end(), pData, pData + length);
			}
			// Process EEEPROM data into mids
			for (const uint8_t addr : m_toEnumerate) {
				if (addr == 0) {
					continue;
				}
				uint16_t idx = addr * 8;
				uint32_t mid = ((uint32_t)eeepromData[idx] | ((uint32_t)eeepromData[idx + 1] << 8) | ((uint32_t)eeepromData[idx + 2] << 16) | ((uint32_t)eeepromData[idx + 3] << 24));
				m_mids.insert(std::make_pair(addr, mid));
			}
		} catch (const std::exception &e) {
			THROW_EXC(std::logic_error, e.what());
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::getRoutingInformation() {
		TRC_FUNCTION_ENTER("");
		if (m_discovered.size() == 0) {
			return;
		}
		std::unique_ptr<IDpaTransactionResult2> result;
		uint16_t startAddr = 0x5000;
		uint16_t totalData = *m_discovered.rbegin() + 1;
		const uint8_t requestCount = totalData / EEEPROM_READ_MAX_LEN;
		const uint8_t remainder = totalData % EEEPROM_READ_MAX_LEN;

		std::vector<uint8_t> aux;

		for (uint8_t i = 0; i < requestCount + 1; i++) {
			uint16_t address = startAddr + i * EEEPROM_READ_MAX_LEN;
			uint8_t length = (i < requestCount) ? EEEPROM_READ_MAX_LEN : remainder;
			uint8_t pData[length];
			memset(pData, 0, length * sizeof(uint8_t));
			eeepromRead(pData, address, length);
			aux.insert(aux.end(), pData, pData + length);
		}

		for (const uint8_t addr : m_discovered) {
			m_vrns.insert(std::make_pair(addr, aux[addr]));
		}

		startAddr = 0x5200;
		aux.clear();

		for (uint8_t i = 0; i < requestCount + 1; i++) {
			uint16_t address = startAddr + i * EEEPROM_READ_MAX_LEN;
			uint8_t length = (i < requestCount) ? EEEPROM_READ_MAX_LEN : remainder;
			uint8_t pData[length];
			memset(pData, 0, length * sizeof(uint8_t));
			eeepromRead(pData, address, length);
			aux.insert(aux.end(), pData, pData + length);
		}

		for (const uint8_t addr : m_discovered) {
			if (addr == 0) {
				m_zones.insert(std::make_pair(addr, 0));
				continue;
			}
			m_zones.insert(std::make_pair(addr, aux[addr] - 1));
		}

		startAddr = 0x5300;
		aux.clear();

		for (uint8_t i = 0; i < requestCount + 1; i++) {
			uint16_t address = startAddr + i * EEEPROM_READ_MAX_LEN;
			uint8_t length = (i < requestCount) ? EEEPROM_READ_MAX_LEN : remainder;
			uint8_t pData[length];
			memset(pData, 0, length * sizeof(uint8_t));
			eeepromRead(pData, address, length);
			aux.insert(aux.end(), pData, pData + length);
		}

		for (const uint8_t addr : m_discovered) {
			m_parents.insert(std::make_pair(addr, aux[addr]));
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::coordinatorEnumeration() {
		uint16_t osBuild = m_coordinatorParams.osBuildWord;
		std::string osVersion = common::device::osVersionString(m_coordinatorParams.osVersionByte, m_coordinatorParams.trMcuType);
		uint16_t dpaVersion = m_coordinatorParams.dpaVerWord;
		uint16_t hwpid = m_coordinatorParams.hwpid;
		uint16_t hwpidVersion = m_coordinatorParams.hwpidVersion;
		UniqueProduct uniqueProduct = std::make_tuple(hwpid, hwpidVersion, osBuild, dpaVersion);
		Product product(hwpid, hwpidVersion, osBuild, osVersion, dpaVersion);
		m_productMap.insert(std::make_pair(uniqueProduct, product));
		m_deviceProductMap.insert(std::make_pair(0, std::make_shared<Product>(m_productMap[uniqueProduct])));
	}

	void IqrfDb::pollEnumeration() {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		// Build DPA requests base
		DpaMessage osReadRequest, peripheralEnumerationRequest;
		DpaMessage::DpaPacket_t osReadPacket, peripheralEnumerationPacket;
		osReadPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
		osReadPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ;
		osReadPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
		peripheralEnumerationPacket.DpaRequestPacket_t.PNUM = PNUM_ENUMERATION;
		peripheralEnumerationPacket.DpaRequestPacket_t.PCMD = CMD_GET_PER_INFO;
		peripheralEnumerationPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
		for (auto it = m_toEnumerate.begin(); it != m_toEnumerate.end(); ) {
			try {
				// Build OS Read request
				osReadPacket.DpaRequestPacket_t.NADR = *it;
				osReadRequest.DataToBuffer(osReadPacket.Buffer, sizeof(TDpaIFaceHeader));
				// Execute OS Read request
				m_dpaService->executeDpaTransactionRepeat(osReadRequest, result, 1);
				DpaMessage osReadResponse = result->getResponse();
				// Process OS Read response
				TPerOSRead_Response osRead = osReadResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSRead_Response;
				uint16_t osBuild = osRead.OsBuild;
				std::string osVersion = common::device::osVersionString(osRead.OsVersion, osRead.McuType);
				// Build peripheral enumeration request
				peripheralEnumerationPacket.DpaRequestPacket_t.NADR = *it;
				peripheralEnumerationRequest.DataToBuffer(peripheralEnumerationPacket.Buffer, sizeof(TDpaIFaceHeader));
				// Execute peripheral enumeration request
				m_dpaService->executeDpaTransactionRepeat(peripheralEnumerationRequest, result, 1);
				DpaMessage peripheralEnumerationResponse = result->getResponse();
				// Process peripheral enumeration request
				TEnumPeripheralsAnswer peripheralEnumeration = peripheralEnumerationResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer;
				uint16_t dpaVersion =  peripheralEnumeration.DpaVersion;
				uint16_t hwpid = peripheralEnumeration.HWPID;
				uint16_t hwpidVersion = peripheralEnumeration.HWPIDver;
				// Build and store product object
				UniqueProduct uniqueProduct = std::make_tuple(hwpid, hwpidVersion, osBuild, dpaVersion);
				if (m_productMap.find(uniqueProduct) == m_productMap.end()) {
					Product product(hwpid, hwpidVersion, osBuild, osVersion, dpaVersion);
					m_productMap.insert(std::make_pair(uniqueProduct, product));
				}
				m_deviceProductMap.insert(std::make_pair(*it, std::make_shared<Product>(m_productMap[uniqueProduct])));
				++it;
			} catch (const std::exception &e) {
				TRC_WARNING("Failed to enumerate node at address " << static_cast<unsigned>(*it) << ": " << e.what());
				m_toEnumerate.erase(*it++);
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::frcEnumeration() {
		TRC_FUNCTION_ENTER("");
		uint8_t maxNodes = 15;
		std::set<uint8_t> onlineNodes = frcPing();
		for (auto it = m_toEnumerate.begin(); it != m_toEnumerate.end();) {
			if (*it == 0) {
				continue;
			}
			if (onlineNodes.find(*it) == onlineNodes.end()) {
				it = m_toEnumerate.erase(it);
			} else {
				++it;
			}
		}
		uint8_t frcCount = std::floor(m_toEnumerate.size() / maxNodes);
		uint8_t frcRemainder = m_toEnumerate.size() % maxNodes;
		std::map<uint8_t, HwpidTuple> hwpidMap;
		std::map<uint8_t, uint16_t> dpaMap;
		std::map<uint8_t, OsTuple> osMap;
		frcHwpid(&hwpidMap, frcCount, maxNodes, frcRemainder);
		frcDpa(&dpaMap, frcCount, maxNodes, frcRemainder);
		frcOs(&osMap, frcCount, maxNodes, frcRemainder);
		for (const uint8_t addr : m_toEnumerate) {
			HwpidTuple hwpidTuple = hwpidMap[addr];
			uint16_t hwpid = std::get<0>(hwpidTuple);
			uint16_t hwpidVersion = std::get<1>(hwpidTuple);
			uint16_t dpa = dpaMap[addr];
			OsTuple osTuple = osMap[addr];
			uint16_t osBuild = std::get<0>(osTuple);
			std::string osVersion = std::get<1>(osTuple);
			UniqueProduct uniqueProduct = std::make_tuple(hwpid, hwpidVersion, osBuild, dpa);
			if (m_productMap.find(uniqueProduct) == m_productMap.end()) {
				Product product(hwpid, hwpidVersion, osBuild, osVersion, dpa);
				m_productMap.insert(std::make_pair(uniqueProduct, product));
			}
			m_deviceProductMap.insert(std::make_pair(addr, std::make_shared<Product>(m_productMap[uniqueProduct])));
		}
		TRC_FUNCTION_LEAVE("");
	}

	///// Requests

	void IqrfDb::eeepromRead(uint8_t* data, const uint16_t &address, const uint8_t &len) {
		std::unique_ptr<IDpaTransactionResult2> result;
		DpaMessage eeepromReadRequest;
		DpaMessage::DpaPacket_t eeepromReadPacket;
		eeepromReadPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
		eeepromReadPacket.DpaRequestPacket_t.PNUM = PNUM_EEEPROM;
		eeepromReadPacket.DpaRequestPacket_t.PCMD = CMD_EEEPROM_XREAD;
		eeepromReadPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
		eeepromReadPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.Address = address;
		eeepromReadPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.ReadWrite.Read.Length = len;
		eeepromReadRequest.DataToBuffer(eeepromReadPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(uint16_t) + sizeof(uint8_t));
		try {
			// Execute EEEPROM read request
			m_dpaService->executeDpaTransactionRepeat(eeepromReadRequest, result, 1);
			DpaMessage eeepromReadResponse = result->getResponse();
			// Process DPA response
			const uint8_t *pData = eeepromReadResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
			for (uint8_t i = 0; i < len; i++) {
				data[i] = pData[i];
			}
		} catch (const std::exception &e) {
			THROW_EXC(std::logic_error, e.what());
		}
	}

	void IqrfDb::frcHwpid(std::map<uint8_t, HwpidTuple> *hwpidMap, uint8_t &frcCount, uint8_t &nodes, uint8_t &remainingNodes) {
		uint16_t memoryAddress = m_memoryAddress + static_cast<uint16_t>(offsetof(TEnumPeripheralsAnswer, HWPID));
		uint8_t processedNodes = 0;
		std::vector<uint8_t> frcData;
		for (uint8_t i = 0, n = frcCount; i <= n; i++) {
			uint8_t numNodes = (uint8_t)(i < frcCount ? nodes : remainingNodes);
			if (numNodes == 0) {
				break;
			}
			uint8_t pData[55] = {0};
			frcSendSelectiveMemoryRead(pData, memoryAddress, PNUM_ENUMERATION, CMD_GET_PER_INFO, numNodes, processedNodes);
			processedNodes += numNodes;
			frcData.insert(frcData.end(), pData + 4, pData + 55);
			if (numNodes > 12) {
				// Execute extra result
				uint8_t extraData[9] = {0};
				frcExtraResult(extraData);
				// Store extra result FRC data
				frcData.insert(frcData.end(), extraData, extraData + 9);
			}
		}
		uint16_t i = 0;
		for (const uint8_t addr : m_toEnumerate) {
			uint16_t hwpid = frcData[i + 1] << 8 | frcData[i];
			uint16_t hwpidVer = frcData[i + 3] << 8 | frcData[i + 2];
			HwpidTuple tuple = std::make_tuple(hwpid, hwpidVer);
			hwpidMap->insert(std::make_pair(addr, tuple));
			i += 4;
		}
	}

	void IqrfDb::frcDpa(std::map<uint8_t, uint16_t> *dpaMap, uint8_t &frcCount, uint8_t &nodes, uint8_t &remainingNodes) {
		TRC_FUNCTION_ENTER("");
		uint16_t memoryAddress = m_memoryAddress + static_cast<uint16_t>(offsetof(TEnumPeripheralsAnswer, DpaVersion));
		uint8_t processedNodes = 0;
		std::vector<uint8_t> frcData;
		for (uint8_t i = 0, n = frcCount; i <= n; i++) {
			uint8_t numNodes = (uint8_t)(i < frcCount ? nodes : remainingNodes);
			if (numNodes == 0) {
				break;
			}
			uint8_t pData[55] = {0};
			frcSendSelectiveMemoryRead(pData, memoryAddress, PNUM_ENUMERATION, CMD_GET_PER_INFO, numNodes, processedNodes);
			processedNodes += numNodes;
			frcData.insert(frcData.end(), pData + 4, pData + 55);
			if (numNodes > 12) {
				uint8_t extraData[9] = {0};
				frcExtraResult(extraData);
				// Store extra result FRC data
				frcData.insert(frcData.end(), extraData, extraData + 9);
			}
		}
		uint16_t i = 0;
		for (const uint8_t addr : m_toEnumerate) {
			uint16_t dpa = frcData[i + 1] << 8 | frcData[i];
			dpaMap->insert(std::make_pair(addr, dpa));
			i += 4;
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::frcOs(std::map<uint8_t, OsTuple> *osMap, uint8_t &frcCount, uint8_t &nodes, uint8_t &remainingNodes) {
		TRC_FUNCTION_ENTER("");
		uint16_t memoryAddress = m_memoryAddress + static_cast<uint16_t>(offsetof(TPerOSRead_Response, OsVersion));
		uint8_t processedNodes = 0;
		std::vector<uint8_t> frcData;
		for (uint8_t i = 0, n = frcCount; i <= n; i++) {
			uint8_t numNodes = (uint8_t)(i < frcCount ? nodes : remainingNodes);
			if (numNodes == 0) {
				break;
			}
			uint8_t pData[55] = {0};
			frcSendSelectiveMemoryRead(pData, memoryAddress, PNUM_OS, CMD_OS_READ, numNodes, processedNodes);
			processedNodes += numNodes;
			frcData.insert(frcData.end(), pData + 4, pData + 55);
			if (numNodes > 12) {
				// Execute extra result
				uint8_t extraData[9] = {0};
				frcExtraResult(extraData);
				// Store extra result FRC data
				frcData.insert(frcData.end(), extraData, extraData + 9);
			}
		}
		uint16_t i = 0;
		for (const uint8_t addr : m_toEnumerate) {
			uint16_t osBuild = frcData[i + 3] << 8 | frcData[i + 2];
			OsTuple tuple = std::make_tuple(osBuild, common::device::osVersionString(frcData[i], frcData[i + 1]));
			osMap->insert(std::make_pair(addr, tuple));
			i+= 4;
		}
		TRC_FUNCTION_LEAVE("");
	}

	const std::set<uint8_t> IqrfDb::frcPing() {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		std::set<uint8_t> onlineNodes;
		try {
			// Build FRC request base
			DpaMessage frcPingRequest;
			DpaMessage::DpaPacket_t frcPingPacket;
			frcPingPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			frcPingPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
			frcPingPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND;
			frcPingPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			// Set FRC command and user data
			frcPingPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = FRC_Ping;
			frcPingPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[0] = 0;
			frcPingPacket.DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData[1] = 0;
			frcPingRequest.DataToBuffer(frcPingPacket.Buffer, sizeof(TDpaIFaceHeader) + 3);
			// Execute FRC request
			m_dpaService->executeDpaTransactionRepeat(frcPingRequest, result, 1);
			DpaMessage frcPingResponse = result->getResponse();
			// Process DPA response
			uint8_t status = frcPingResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
			if (status >= 0xef) {
				THROW_EXC_TRC_WAR(std::logic_error, "FRC response error, status: " << status);
			}
			std::vector<uint8_t> data;
			const uint8_t *frcData = frcPingResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData;
			// Store addresses of online nodes
			for (uint8_t i = 1, n = MAX_ADDRESS; i <= n; i++) {
				if ((frcData[i / 8] & (1 << (i % 8))) != 0) {
					onlineNodes.insert(i);
				}
			}
		} catch (const std::exception &e) {
			THROW_EXC(std::logic_error, e.what());
		}
		TRC_FUNCTION_LEAVE("");
		return onlineNodes;
	}

	void IqrfDb::frcSendSelectiveMemoryRead(uint8_t* data, const uint16_t &address, const uint8_t &pnum, const uint8_t &pcmd, const uint8_t &numNodes, const uint8_t &processedNodes) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		try {
			// Build FRC request base
			DpaMessage frcSendSelectiveRequest;
			DpaMessage::DpaPacket_t frcSendSelectivePacket;
			frcSendSelectivePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			frcSendSelectivePacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
			frcSendSelectivePacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
			frcSendSelectivePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			// Set FRC command and user data
			frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_MemoryRead4B;
			frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0] = 0;
			frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[1] = 0;
			// Set FRC memory read
			frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[2] = address & 0xff;
			frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[3] = address >> 8;
			frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[4] = pnum;
			frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[5] = pcmd;
			frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[6] = 0;
			// Set selected nodes
			std::vector<uint8_t> nodes = IqrfDbAux::selectNodes(m_toEnumerate, processedNodes, numNodes);
			std::copy(nodes.begin(), nodes.end(), frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes);
			frcSendSelectiveRequest.DataToBuffer(frcSendSelectivePacket.Buffer, sizeof(TDpaIFaceHeader) + 38);
			// Execute FRC request
			m_dpaService->executeDpaTransactionRepeat(frcSendSelectiveRequest, result, 1);
			DpaMessage frcSendSelectiveResponse = result->getResponse();
			// Process DPA response
			uint8_t status = frcSendSelectiveResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
			if (status >= 0xef) {
				THROW_EXC_TRC_WAR(std::logic_error, "FRC response error, status: " << std::to_string(status));
			}
			const uint8_t *pData = frcSendSelectiveResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData;
			for (uint8_t i = 0; i < 55; i++) {
				data[i] = pData[i];
			}
		} catch (const std::exception &e) {
			THROW_EXC(std::logic_error, e.what());
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::frcExtraResult(uint8_t *data) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		try {
			// Build FRC extra result
			DpaMessage frcExtraResultRequest;
			DpaMessage::DpaPacket_t frcExtraResultPacket;
			frcExtraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			frcExtraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
			frcExtraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
			frcExtraResultPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			frcExtraResultRequest.DataToBuffer(frcExtraResultPacket.Buffer, sizeof(TDpaIFaceHeader));
			// Execute DPA request
			m_dpaService->executeDpaTransactionRepeat(frcExtraResultRequest, result, 1);
			DpaMessage frcExtraResultResponse = result->getResponse();
			const uint8_t *pData = frcExtraResultResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
			for (uint8_t i = 0; i < 9; i++) {
				data[i] = pData[i];
			}
		} catch (const std::exception &e) {
			THROW_EXC(std::logic_error, e.what());
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::productPackageEnumeration() {
		TRC_FUNCTION_ENTER("");
		using namespace sqlite_orm;
		if (m_deviceProductMap.count(0) != 0) {
			m_toEnumerate.insert(0);
		}
		for (const uint8_t addr : m_toEnumerate) {
			auto &product = m_deviceProductMap[addr];
			bool validProduct = product->isValid();
			if (!validProduct) {
				continue;
			}
			uint16_t hwpid = product->getHwpid();
			uint16_t hwpidVersion = product->getHwpidVersion();
			uint16_t osBuild = product->getOsBuild();
			uint16_t dpaVersion = product->getDpaVersion();
			std::shared_ptr<IJsCacheService::Package> package = m_cacheService->getPackage(hwpid, hwpidVersion, osBuild, dpaVersion);
			if (package == nullptr) {
				// try to find db product
				auto dbProduct = this->getProduct(hwpid, hwpidVersion, osBuild, dpaVersion);
				if (dbProduct != nullptr) {
					product->setHandlerUrl(dbProduct->getHandlerUrl());
					product->setHandlerHash(dbProduct->getHandlerHash());
					product->setNotes(dbProduct->getNotes());
					product->setCustomDriver(dbProduct->getCustomDriver());
					product->setPackageId(dbProduct->getPackageId());
					auto driverIds = this->getDriverIdsByProduct(dbProduct->getId());
					for (auto id : driverIds) {
						product->drivers.insert(id);
					}
					continue;
				}
				// try hwpid 0 package
				package = m_cacheService->getPackage(0, 0, osBuild, dpaVersion);
			}
			if (package == nullptr) {
				// try to find package for lower DPA
				uint16_t dpa = dpaVersion - 1;
				while (package == nullptr && dpa >= 768) {
					package = m_cacheService->getPackage(0, 0, osBuild, dpa);
					dpa--;
				}
			}
			if (package == nullptr) {
				TRC_WARNING("Cannot find package for: " << NAME_PAR(nadr, addr) << NAME_PAR(hwpid, 0) << NAME_PAR(hwpidVer, 0) << NAME_PAR(osBuild, osBuild) << " any DPA");
				continue;
			}

			if (package->m_handlerUrl.length() != 0) {
				product->setHandlerUrl(std::make_shared<std::string>(package->m_handlerUrl));
			}
			if (package->m_handlerHash.length() != 0) {
				product->setHandlerHash(std::make_shared<std::string>(package->m_handlerHash));
			}
			if (package->m_notes.length() != 0) {
				product->setNotes(std::make_shared<std::string>(package->m_notes));
			}
			if (package->m_driver.length() != 0) {
				product->setCustomDriver(std::make_shared<std::string>(package->m_driver));
			}
			product->setPackageId(package->m_packageId);
			for (auto &item : package->m_stdDriverVect) {
				int16_t per = item.getId();
				double version = item.getVersion();
				auto dbDriver = this->getDriver(per, version);
				if (dbDriver == nullptr) {
					Driver driver(item.getName(), per, version, item.getVersionFlags(), *item.getNotes(), *item.getDriver());
					uint32_t driverId = this->insertDriver(driver);
					product->drivers.insert(driverId);
				} else {
					product->drivers.insert(dbDriver->getId());
				}
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::updateDatabaseProducts() {
		TRC_FUNCTION_ENTER("");
		using namespace sqlite_orm;
		m_db->begin_transaction();
		for (auto &deleteId : m_toDelete) {
			this->removeDevice(deleteId);
		}
		uint32_t productId = -1;
		for (auto &addr : m_toEnumerate) {
			ProductPtr &product = m_deviceProductMap[addr];
			try {
				// create a new product or update existing
				auto dbProduct = this->getProduct(
					product->getHwpid(),
					product->getHwpidVersion(),
					product->getOsBuild(),
					product->getDpaVersion()
				);

				// store new product or use existing
				if (dbProduct  == nullptr) {
					productId = this->insertProduct(*product.get());
				} else {
					productId = dbProduct->getId();
				}

				// update product drivers, reuse dbProductDrivers to delete old unused drivers by erasing intersecting elements
				auto dbProductDrivers = this->getProductDriversIds(productId);
				if (dbProductDrivers != product->drivers) {
					for (auto &driverId : product->drivers) {
						if (dbProductDrivers.count(driverId)) {
							// product driver in db, do not add and remove from old db set
							dbProductDrivers.erase(driverId);
							continue;
						} else {
							// add new product driver
							ProductDriver productDriver(productId, driverId);
							this->insertProductDriver(productDriver);
						}
					}
					// remove old unused product drivers
					for (auto &driverId : dbProductDrivers) {
						// remove old set
						this->removeProductDriver(productId, driverId);
					}
				}

				// create a new device or update existing
				bool discovered = m_discovered.find(addr) != m_discovered.end();
				uint32_t mid = m_mids[addr];
				uint8_t vrn = discovered ? m_vrns[addr] : 0;
				uint8_t zone = discovered ? m_zones[addr] : 0;
				std::shared_ptr<uint8_t> parent = discovered ? std::make_shared<uint8_t>(m_parents[addr]) : nullptr;
				auto dbDevice = this->getDevice(addr);
				if (dbDevice == nullptr) {
					// create new
					Device device(addr, discovered, mid, vrn, zone, parent);
					device.setProductId(productId);
					this->insertDevice(device);
				} else {
					// update existing
					Device device = *dbDevice.get();
					if (device.isDiscovered() != discovered) {
						device.setDiscovered(discovered);
					}
					if (device.getMid() != mid) {
						device.setMid(mid);
					}
					if (device.getVrn() != vrn) {
						device.setVrn(vrn);
					}
					if (device.getZone() != zone) {
						device.setZone(zone);
					}
					if (device.getParent() != parent) {
						device.setParent(parent);
					}
					if (device.getProductId() != productId) {
						device.setProductId(productId);
					}
					this->updateDevice(device);
				}
			} catch (const std::system_error &e) {
				CATCH_EXC_TRC_WAR(const std::system_error, e, "Failed to insert entity to database - [" << e.code() << "]: " << e.what());
				m_db->rollback();
				return;
			}
		}
		m_db->commit();
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::standardEnumeration() {
		TRC_FUNCTION_ENTER("");
		using namespace sqlite_orm;
		// select devices to enumerate
		std::map<uint32_t, uint8_t> devices;
		for (auto &device : this->getDevices()) {
			if (!device.isEnumerated() || m_params.reenumerate) {
				devices.insert(std::make_pair(device.getId(), device.getAddress()));
			}
		}

		for (auto &device : devices) {
			uint32_t deviceId = device.first;
			uint8_t address = device.second;

			// begin transaction
			m_db->begin_transaction();
			try {
				if (this->deviceImplementsPeripheral(deviceId, PERIPHERAL_BINOUT)) {
					binoutEnumeration(deviceId, address);
				} else {
					this->removeBinaryOutput(deviceId);
				}
				if (this->deviceImplementsPeripheral(deviceId, PERIPHERAL_DALI)) {
					daliEnumeration(deviceId);
				} else {
					this->removeDali(deviceId);
				}
				if (this->deviceImplementsPeripheral(deviceId, PERIPHERAL_LIGHT)) {
					lightEnumeration(deviceId, address);
				} else {
					this->removeLight(deviceId);
				}
				if (this->deviceImplementsPeripheral(deviceId, PERIPHERAL_SENSOR)) {
					sensorEnumeration(address);
				} else {
					this->removeDeviceSensors(deviceId);
				}
				// set as enumerated
				auto dbDevice = this->getDevice(deviceId);
				dbDevice->setEnumerated(true);
				this->updateDevice(*dbDevice.get());
				m_db->commit();
			}  catch (const std::exception &e) {
				m_db->rollback();
				CATCH_EXC_TRC_WAR(std::exception, e, e.what());
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::binoutEnumeration(const uint32_t &deviceId, const uint8_t &address) {
		TRC_FUNCTION_ENTER("");
		using namespace sqlite_orm;
		std::unique_ptr<IDpaTransactionResult2> result;
		try {
			// Build binary output enumerate request
			DpaMessage binoutEnumerateRequest;
			DpaMessage::DpaPacket_t binoutEnumeratePacket;
			binoutEnumeratePacket.DpaRequestPacket_t.NADR = address;
			binoutEnumeratePacket.DpaRequestPacket_t.PNUM = PERIPHERAL_BINOUT;
			binoutEnumeratePacket.DpaRequestPacket_t.PCMD = 0x3E;
			binoutEnumeratePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			binoutEnumerateRequest.DataToBuffer(binoutEnumeratePacket.Buffer, sizeof(TDpaIFaceHeader));
			// Execute DPA request
			m_dpaService->executeDpaTransactionRepeat(binoutEnumerateRequest, result, 1);
		} catch (const std::exception &e) {
			THROW_EXC(std::logic_error, e.what());
		}
		DpaMessage binoutEnumerateResponse = result->getResponse();
		const uint8_t count = binoutEnumerateResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[0];

		auto dbBO = this->getBinaryOutputByDeviceId(deviceId);
		if (dbBO == nullptr) {
			BinaryOutput binaryOutput(deviceId, count);
			this->inseryBinaryOutput(binaryOutput);
		} else {
			dbBO->setCount(count);
			this->updateBinaryOutput(*dbBO.get());
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::daliEnumeration(const uint32_t &deviceId) {
		TRC_FUNCTION_ENTER("");
		using namespace sqlite_orm;

		if (this->getDaliByDeviceId(deviceId) == nullptr) {
			Dali dali(deviceId);
			this->insertDali(dali);
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::lightEnumeration(const uint32_t &deviceId, const uint8_t &address) {
		TRC_FUNCTION_ENTER("");
		using namespace sqlite_orm;
		std::unique_ptr<IDpaTransactionResult2> result;
		try {
			// Build light enumerate request
			DpaMessage lightEnumerateRequest;
			DpaMessage::DpaPacket_t lightEnumeratePacket;
			lightEnumeratePacket.DpaRequestPacket_t.NADR = address;
			lightEnumeratePacket.DpaRequestPacket_t.PNUM = PERIPHERAL_LIGHT;
			lightEnumeratePacket.DpaRequestPacket_t.PCMD = 0x3E;
			lightEnumeratePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			lightEnumerateRequest.DataToBuffer(lightEnumeratePacket.Buffer, sizeof(TDpaIFaceHeader));
			// Execute DPA request
			m_dpaService->executeDpaTransactionRepeat(lightEnumerateRequest, result, 1);
		} catch (const std::exception &e) {
			THROW_EXC(std::logic_error, e.what());
		}
		DpaMessage lightEnumerateResponse = result->getResponse();
		const uint8_t count = lightEnumerateResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[0];

		auto dbLight = this->getLightByDeviceId(deviceId);
		if (dbLight == nullptr) {
			Light light(deviceId, count);
			this->insertLight(light);
		} else {
			dbLight->setCount(count);
			this->updateLight(*dbLight.get());
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::sensorEnumeration(const uint8_t &address) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		sensor::jsdriver::Enumerate sensorEnum(m_renderService, address);
		m_dpaService->executeDpaTransactionRepeat(sensorEnum.getRequest(), result, 1);
		sensorEnum.processDpaTransactionResult(std::move(result));

		auto &sensors = sensorEnum.getSensors();

		uint8_t cnt[255] = {0};

		for (auto &item : sensors) {
			uint32_t sensorId;
			uint8_t type = item->getType();
			bool breakdown = item->hasBreakdown();
			std::string name = breakdown ? item->getBreakdownName() : item->getName();
			auto dbSensor = this->getSensor(type, name);
			if (dbSensor != nullptr) {
				sensorId = dbSensor->getId();
			} else {
				// Get sensor information, breakdown if possible
				std::string shortname = breakdown ? item->getBreakdownShortName() : item->getShortName();
				std::string unit = breakdown ? item->getBreakdownUnit() : item->getUnit();
				uint8_t decimals = breakdown ? item->getBreakdownDecimalPlaces() : item->getDecimalPlaces();
				// FRCs
				auto &frcs = item->getFrcs();
				bool frc2Bit = frcs.find(iqrf::sensor::STD_SENSOR_FRC_2BITS) != frcs.end();
				bool frc1Byte = frcs.find(iqrf::sensor::STD_SENSOR_FRC_1BYTE) != frcs.end();
				bool frc2Byte = frcs.find(iqrf::sensor::STD_SENSOR_FRC_2BYTES) != frcs.end();
				bool frc4Byte = frcs.find(iqrf::sensor::STD_SENSOR_FRC_4BYTES) != frcs.end();
				Sensor sensor(type, name, shortname, unit, decimals, frc2Bit, frc1Byte, frc2Byte, frc4Byte);
				// Store new sensor and get ID
				sensorId = this->insertSensor(sensor);
			}

			// store device and sensor
			const uint8_t index = item->getIdx();
			auto dbDeviceSensor = this->getDeviceSensor(address, sensorId, index);
			if (dbDeviceSensor == nullptr) {
				DeviceSensor deviceSensor(address, type, index, cnt[type], sensorId, nullptr);
				this->insertDeviceSensor(deviceSensor);
				cnt[type] += 1;
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::loadCoordinatorDrivers() {
		TRC_FUNCTION_ENTER("");
		std::string wrapper = loadWrapper();
		m_coordinatorParams = m_dpaService->getCoordinatorParameters();
		uint16_t osBuild = m_coordinatorParams.osBuildWord;
		uint16_t dpaVersion = m_coordinatorParams.dpaVerWord;

		auto drivers = m_cacheService->getDrivers(m_coordinatorParams.osBuild, m_coordinatorParams.dpaVerWordAsStr);

		if (drivers.size() == 0) {
			std::ostringstream oss;
			oss << std::endl
				<< "Failed to load drivers for OS " << m_coordinatorParams.osBuild
				<< ", DPA " << m_coordinatorParams.dpaVerWordAsStr;
			std::cout << oss.str() << std::endl;
			TRC_WARNING(oss.str());
			oss.str(std::string());

			const auto &osDpa = m_cacheService->getOsDpa();

			auto itr = osDpa.find(osBuild);
			if (itr == osDpa.end()) {
				int provisoryOsBuild = -1;
				auto revisionItr = osDpa.rbegin();
				while (revisionItr != osDpa.rend()) {
					if (revisionItr->first <= osBuild) {
						provisoryOsBuild = revisionItr->first;
						break;
					}
					revisionItr++;
				}
				if (provisoryOsBuild < 0) {
					provisoryOsBuild = osDpa.begin()->first;
				}
				osBuild = provisoryOsBuild;
				itr = osDpa.find(osBuild);
			}

			if (itr == osDpa.end()) {
				THROW_EXC_TRC_WAR(std::logic_error, "Inconsistent OS-DPA map: " << m_coordinatorParams.osBuild);
			}

			auto revisionItr = itr->second.rbegin();
			while (revisionItr != itr->second.rend()) {
				if (*revisionItr <= dpaVersion) {
					dpaVersion = *revisionItr;

					drivers = m_cacheService->getDrivers(common::device::osBuildString(osBuild), common::device::dpaVersionHexaString(dpaVersion));
					if (drivers.size() > 0) {
						oss << std::endl
							<< "Loaded drivers for OS " << common::device::osBuildString(osBuild)
							<< ", DPA " << common::device::dpaVersionHexaString(dpaVersion);
						std::cout << oss.str() << std::endl;
						TRC_WARNING(oss.str());
						break;
					}
				}
				revisionItr++;
			}

		}

		std::stringstream ss;
		std::set<uint32_t> driversToLoad;

		for (auto &driver : drivers) {
			int id = driver.first;
			double version = 0;
			driversToLoad.insert(id);

			if (driver.second.size() > 0) {
				// use latest
				version = driver.second.rbegin()->first;
			} else {
				TRC_WARNING("No driver version found for driver ID: " << id);
			}

			std::shared_ptr<IJsCacheService::StdDriver> cacheDriver = m_cacheService->getDriver(id, version);
			if (cacheDriver != nullptr) {
				ss << *cacheDriver->getDriver();
			} else {
				TRC_WARNING("No driver found in cache for ID: " << id << ", version: " << version);
			}
		}

		ss << wrapper;

		m_renderService->loadContextCode(IJsRenderService::HWPID_DEFAULT_MAPPING, ss.str(), driversToLoad);

		auto customDrivers = m_cacheService->getCustomDrivers(m_coordinatorParams.osBuild, m_coordinatorParams.dpaVerWordAsStr);

		for (auto &driver : customDrivers) {
			std::string customDriverToLoad = ss.str();
			customDriverToLoad += driver.second.rbegin()->second;
			m_renderService->loadContextCode(IJsRenderService::HWPID_MAPPING_SPACE - driver.first, customDriverToLoad, driversToLoad);
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::loadProductDrivers() {
		TRC_FUNCTION_ENTER("");
		using namespace sqlite_orm;
		std::string wrapper = loadWrapper();

		try {
			std::map<uint32_t, std::set<uint32_t>> productsDrivers = this->getProductsDriversIdMap();
			std::set<uint8_t> reloadDevices;

			uint32_t coordinatorProductId = this->getCoordinatorProductId();

			// check if device drivers need to be reloaded
			for (auto &pd : productsDrivers) {
				const uint8_t productId = pd.first;
				const std::set<uint32_t> &dbDrivers = pd.second;
				auto currentDrivers = m_renderService->getDriverIdSet(productId);
				if (currentDrivers != dbDrivers) {
					reloadDevices.insert(productId);
				}
			}

			if (reloadDevices.size() > 0) {
				for (uint32_t productId : reloadDevices) {
					std::vector<Driver> drivers;
					if (productId == coordinatorProductId) { // ensure standard FRC backwards compatibility
						drivers = this->getLatestDrivers();
					} else {
						drivers = this->getDriversByProduct(productId);
					}
					std::string customDriver = this->getProductCustomDriver(productId);

					std::ostringstream drv, adr;
					std::stringstream ss;
					std::set<uint32_t> driverSet;
					for (auto driver : drivers) {
						driverSet.insert(driver.getId());
						ss << driver.getDriver() << std::endl;
						drv << '[' << driver.getPeripheralNumber() << ',' << std::fixed << std::setprecision(2) << driver.getVersion() << ']';
					}

					ss << customDriver << std::endl;
					ss << wrapper << std::endl;
					bool success = m_renderService->loadContextCode(productId, ss.str(), driverSet);

					if (!success) {
						TRC_WARNING_CHN(
							33,
							"iqrf::JsCache",
							"Failed to load drivers for deviceId: " << productId << std::endl
						);
						continue;
					}

					std::vector<uint8_t> addresses = this->getProductDeviceAddresses(productId);

					for (auto addr : addresses) {
						m_renderService->mapAddressToContext(addr, productId);
						adr << std::to_string(addr) << ", ";
					}

					TRC_INFORMATION_CHN(33, "iqrf::JsCache", "Loading drivers for context: "
						<< std::endl << "nadr: " << adr.str()
						<< std::endl << "drv:  " << drv.str()
						<< std::endl
            		);
				}
			}
		} catch (std::exception &e) {
			CATCH_EXC_TRC_WAR(std::exception, e, "Failed to load drivers: " << e.what());
		}

		TRC_FUNCTION_LEAVE("");
	}

	///// Auxiliary functions /////

	std::string IqrfDb::loadWrapper() {
		std::string path = m_launchService->getDataDir() + "/javaScript/DaemonWrapper.js";
		std::ifstream file(path);
		if (!file.is_open()) {
			THROW_EXC_TRC_WAR(std::logic_error, "Failed to open file wrapper file: " << path);
		}
		std::stringstream ss;
		ss << file.rdbuf();
		return ss.str();
	}

	void IqrfDb::clearAuxBuffers() {
		TRC_FUNCTION_ENTER("");
		m_toEnumerate.clear();
		m_toDelete.clear();
		m_discovered.clear();
		m_mids.clear();
		m_vrns.clear();
		m_zones.clear();
		m_parents.clear();
		m_productMap.clear();
		m_deviceProductMap.clear();
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::waitForExclusiveAccess() {
		std::unique_lock<std::mutex> lock(m_enumMutex);
		while (m_exclusiveAccessCv.wait_for(lock, std::chrono::seconds(1), [&] {
			return m_dpaService->hasExclusiveAccess();
		}));
		m_exclusiveAccess = m_dpaService->getExclusiveAccess();
		TRC_DEBUG("Exclusive access acquired.");
	}

	void IqrfDb::resetExclusiveAccess() {
		std::unique_lock<std::mutex> lock(m_enumMutex);
		if (m_exclusiveAccess != nullptr) {
			m_exclusiveAccess.reset();
			TRC_DEBUG("Exclusive access released.");
		}
	}

	///// Component instance lifecycle methods /////

	void IqrfDb::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************" << std::endl <<
			"IqrfDb instance activate" << std::endl <<
			"******************************"
		);
		modify(props);
		m_cacheService->registerCacheReloadedHandler(m_instance, [&]() {
			reloadDrivers();
		});
		m_dpaService->registerAnyMessageHandler(m_instance, [&](const DpaMessage &msg) {
			analyzeDpaMessage(msg);
		});
		initializeDatabase();
		reloadDrivers();

		m_enumRun = false;
		m_enumRepeat = false;
		m_enumThreadRun = false;
		if (m_enumerateOnLaunch) {
			m_enumRun = true;
		}
		if (m_enumerateOnLaunch || m_autoEnumerateBeforeInvoked) {
			EnumParams parameters {true, true};
			startEnumerationThread(parameters);
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		using namespace rapidjson;
		// path to db file
		m_dbPath = m_launchService->getDataDir() + "/DB/IqrfDb.db";
		// read configuration parameters
		const Document &doc = props->getAsJson();
		m_instance = Pointer("/instance").Get(doc)->GetString();
		m_autoEnumerateBeforeInvoked = Pointer("/autoEnumerateBeforeInvoked").Get(doc)->GetBool();
		m_enumerateOnLaunch = Pointer("/enumerateOnLaunch").Get(doc)->GetBool();
		m_metadataTomessages = Pointer("/metadataToMessages").Get(doc)->GetBool();
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************" << std::endl <<
			"IqrfDb instance deactivate" << std::endl <<
			"******************************"
		);
		m_enumThreadRun = false;
		stopEnumerationThread();
		m_cacheService->unregisterCacheReloadedHandler(m_instance);
		m_dpaService->unregisterAnyMessageHandler(m_instance);
		clearAuxBuffers();
		TRC_FUNCTION_LEAVE("");
	}

	///// Service interfaces /////

	void IqrfDb::attachInterface(IIqrfDpaService *iface) {
		m_dpaService = iface;
	}

	void IqrfDb::detachInterface(IIqrfDpaService *iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void IqrfDb::attachInterface(IJsCacheService *iface) {
		m_cacheService = iface;
	}

	void IqrfDb::detachInterface(IJsCacheService *iface) {
		if (m_cacheService == iface) {
			m_cacheService = nullptr;
		}
	}

	void IqrfDb::attachInterface(IJsRenderService *iface) {
		m_renderService = iface;
	}

	void IqrfDb::detachInterface(IJsRenderService *iface) {
		if (m_renderService == iface) {
			m_renderService = nullptr;
		}
	}

	void IqrfDb::attachInterface(shape::ILaunchService *iface) {
		m_launchService = iface;
	}

	void IqrfDb::detachInterface(shape::ILaunchService *iface) {
		if (m_launchService == iface) {
			m_launchService = nullptr;
		}
	}

	void IqrfDb::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void IqrfDb::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}

}
