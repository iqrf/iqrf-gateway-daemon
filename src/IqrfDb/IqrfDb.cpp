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

	///// Transactions

	void IqrfDb::beginTransaction() {
		m_db->begin_transaction();
	}

	void IqrfDb::finishTransaction() {
		m_db->commit();
	}

	void IqrfDb::cancelTransaction() {
		m_db->rollback();
	}

	///// Binary outputs

	std::unique_ptr<BinaryOutput> IqrfDb::getBinaryOutput(const uint32_t &id) {
		return m_db->get_pointer<BinaryOutput>(id);
	}

	std::unique_ptr<BinaryOutput> IqrfDb::getBinaryOutputByDevice(const uint32_t &deviceId) {
		auto entities = m_db->get_all<BinaryOutput>(
			where(c(&BinaryOutput::getDeviceId) == deviceId)
		);
		if (entities.size() == 0) {
			return nullptr;
		}
		return std::make_unique<BinaryOutput>(entities[0]);
	}

	uint32_t IqrfDb::insertBinaryOutput(BinaryOutput &binaryOutput) {
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

	std::map<uint8_t, uint8_t> IqrfDb::getBinaryOutputCountMap() {
		auto rows = m_db->select(columns(&Device::getAddress, &BinaryOutput::getCount),
			inner_join<BinaryOutput>(on(c(&BinaryOutput::getDeviceId) == &Device::getId))
		);
		std::map<uint8_t, uint8_t> binaryOutputs;
		for (auto &row : rows) {
			binaryOutputs.insert(std::make_pair(std::get<0>(row), std::get<1>(row)));
		}
		return binaryOutputs;
	}

	///// DALIs

	std::unique_ptr<Dali> IqrfDb::getDaliByDevice(const uint32_t &deviceId) {
		auto entities = m_db->get_all<Dali>(
			where(c(&Dali::getDeviceId) == deviceId)
		);
		if (entities.size() == 0) {
			return nullptr;
		}
		return std::make_unique<Dali>(entities[0]);
	}

	uint32_t IqrfDb::insertDali(Dali &dali) {
		return m_db->insert<Dali>(dali);
	}

	void IqrfDb::removeDali(const uint32_t &deviceId) {
		m_db->remove<Dali>(
			where(c(&Dali::getDeviceId) == deviceId)
		);
	}

	std::set<uint8_t> IqrfDb::getDaliDeviceAddresses() {
		auto rows = m_db->select(&Device::getAddress,
			inner_join<Dali>(on(c(&Dali::getDeviceId) == &Device::getId))
		);
		std::set<uint8_t> dalis;
		for (auto &row : rows) {
			dalis.insert(row);
		}
		return dalis;
	}

	///// Devices

	std::vector<Device> IqrfDb::getAllDevices() {
		return m_db->get_all<Device>();
	}

	std::unique_ptr<Device> IqrfDb::getDevice(const uint32_t &id) {
		return m_db->get_pointer<Device>(id);
	}

	std::unique_ptr<Device> IqrfDb::getDevice(const uint8_t &address) {
		auto entities = m_db->get_all<Device>(
			where(c(&Device::getAddress) == address)
		);
		if (entities.size() == 0) {
			return nullptr;
		}
		return std::make_unique<Device>(entities[0]);
	}

	uint32_t IqrfDb::insertDevice(Device &device) {
		return m_db->insert<Device>(device);
	}

	void IqrfDb::updateDevice(Device &device) {
		m_db->update<Device>(device);
	}

	void IqrfDb::removeDevice(const uint32_t &id) {
		m_db->remove<Device>(id);
	}

	bool IqrfDb::deviceImplementsPeripheral(const uint32_t &id, int16_t peripheral) {
		auto entities = m_db->select(&Driver::getId,
			inner_join<ProductDriver>(on(c(&ProductDriver::getDriverId) == &Driver::getId)),
			inner_join<Device>(on(c(&Device::getProductId) == &ProductDriver::getProductId)),
			where(
				c(&Device::getId) == id
				and c(&Driver::getPeripheralNumber) == peripheral
			)
		);
		return entities.size() > 0;
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

	///// Device sensors

	std::unique_ptr<DeviceSensor> IqrfDb::getDeviceSensor(const uint32_t &id) {
		return m_db->get_pointer<DeviceSensor>(id);
	}

	std::unique_ptr<DeviceSensor> IqrfDb::getDeviceSensor(const uint8_t &address, const uint32_t &sensorId, const uint8_t &index) {
		auto entities = m_db->get_all<DeviceSensor>(
			where(
				c(&DeviceSensor::getAddress) == address
				and c(&DeviceSensor::getSensorId) == sensorId
				and c(&DeviceSensor::getGlobalIndex) == index
			)
		);
		if (entities.size() == 0) {
			return nullptr;
		}
		return std::make_unique<DeviceSensor>(entities[0]);
	}

	std::unique_ptr<iqrf::db::DeviceSensor> IqrfDb::getDeviceSensorByGlobalIndex(const uint8_t &address, const uint8_t &index) {
		auto entities = m_db->get_all<DeviceSensor>(
			where(
				c(&DeviceSensor::getAddress) == address
				and c(&DeviceSensor::getGlobalIndex) == index
			)
		);
		if (entities.size() == 0) {
			throw std::logic_error("Device at address " + std::to_string(address) + " does not implement sensor at index " + std::to_string(index));
		}
		return std::make_unique<DeviceSensor>(entities[0]);
	}

	std::unique_ptr<DeviceSensor> IqrfDb::getDeviceSensorByTypeIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) {
		auto entities = m_db->get_all<DeviceSensor>(
			where(
				c(&DeviceSensor::getAddress) == address
				and c(&DeviceSensor::getType) == type
				and c(&DeviceSensor::getTypeIndex) == index
			)
		);
		if (entities.size() == 0) {
			throw std::logic_error("Device at address " + std::to_string(address) + " does not implement sensor type " + std::to_string(type) + " index " + std::to_string(index));
		}
		return std::make_unique<DeviceSensor>(entities[0]);
	}

	void IqrfDb::insertDeviceSensor(DeviceSensor &deviceSensor) {
		m_db->replace<DeviceSensor>(deviceSensor);
	}

	void IqrfDb::updateDeviceSensor(DeviceSensor &deviceSensor) {
		m_db->update<DeviceSensor>(deviceSensor);
	}

	void IqrfDb::removeDeviceSensors(const uint8_t &address) {
		m_db->remove_all<DeviceSensor>(where(c(&DeviceSensor::getAddress) == address));
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

	uint8_t IqrfDb::getDeviceSensorGlobalIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) {
		auto deviceSensors = m_db->get_all<DeviceSensor>(
			where(
				c(&DeviceSensor::getAddress) == address
				and c(&DeviceSensor::getType) == type
				and c(&DeviceSensor::getTypeIndex) == index
			)
		);
		if (deviceSensors.size() == 0) {
			throw std::logic_error("Device at address " + std::to_string(address)
				+ " does not implement sensor of type " + std::to_string(type)
				+ " at index " + std::to_string(index)
			);
		}
		return deviceSensors[0].getGlobalIndex();
	}

	///// Drivers

	std::unique_ptr<Driver> IqrfDb::getDriver(const int16_t &peripheral, const double &version) {
		auto entities = m_db->get_all<Driver>(
			where(
				c(&Driver::getPeripheralNumber) == peripheral
				and c(&Driver::getVersion) == version
			)
		);
		if (entities.size() == 0) {
			return nullptr;
		}
		return std::make_unique<Driver>(entities[0]);
	}

	std::vector<Driver> IqrfDb::getDriversByProduct(const uint32_t &productId) {
		auto productDrivers = m_db->get_all<ProductDriver>(where(c(&ProductDriver::getProductId) == productId));
		std::vector<Driver> drivers;
		for (auto &pd : productDrivers) {
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

	std::vector<iqrf::db::Driver> IqrfDb::getLatestDrivers() {
		return m_db->get_all<Driver>(max(&Driver::getVersion), group_by(&Driver::getPeripheralNumber));
	}

	uint32_t IqrfDb::insertDriver(Driver &driver) {
		return m_db->insert<Driver>(driver);
	}

	///// Lights

	std::unique_ptr<Light> IqrfDb::getLight(const uint32_t &id) {
		return m_db->get_pointer<Light>(id);
	}

	std::unique_ptr<Light> IqrfDb::getLightByDevice(const uint32_t &deviceId) {
		auto entities = m_db->get_all<Light>(
			where(c(&Light::getDeviceId) == deviceId)
		);
		if (entities.size() == 0) {
			return nullptr;
		}
		return std::make_unique<Light>(entities[0]);
	}

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

	std::map<uint8_t, uint8_t> IqrfDb::getLightCountMap() {
		auto rows = m_db->select(columns(&Device::getAddress, &Light::getCount),
			inner_join<Light>(on(c(&Light::getDeviceId) == &Device::getId))
		);
		std::map<uint8_t, uint8_t> lights;
		for (auto &row : rows) {
			lights.insert(std::make_pair(std::get<0>(row), std::get<1>(row)));
		}
		return lights;
	}

	///// Products

	std::unique_ptr<Product> IqrfDb::getProduct(const uint32_t &id) {
		return m_db->get_pointer<Product>(id);
	}

	std::unique_ptr<Product> IqrfDb::getProduct(const uint16_t &hwpid, const uint16_t &hwpidVer, const uint16_t &osBuild, const uint16_t &dpa) {
		auto entities = m_db->get_all<Product>(
			where(
				c(&Product::getHwpid) == hwpid
				and c(&Product::getHwpidVersion) == hwpidVer
				and c(&Product::getOsBuild) == osBuild
				and c(&Product::getDpaVersion) == dpa
			)
		);
		if (entities.size() == 0) {
			return nullptr;
		}
		return std::make_unique<Product>(entities[0]);
	}

	uint32_t IqrfDb::insertProduct(Product &product) {
		return m_db->insert<Product>(product);
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

	///// Product drivers

	void IqrfDb::insertProductDriver(ProductDriver &productDriver) {
		m_db->replace(productDriver);
	}

	std::map<uint32_t, std::set<uint32_t>> IqrfDb::getProductsDriversIdMap() {
		auto rows = m_db->select(columns(&Product::getId, &Driver::getId),
			inner_join<ProductDriver>(on(c(&ProductDriver::getDriverId) == &Driver::getId)),
			inner_join<Product>(on(c(&Product::getId) == &ProductDriver::getProductId))
		);

		std::map<uint32_t, std::set<uint32_t>> productDrivers;

		for (auto &row : rows) {
			uint32_t productId = std::get<0>(row);
			uint32_t driverId = std::get<1>(row);
			if (productDrivers.count(productId) == 0) {
				productDrivers.insert(std::make_pair(productId, std::set<uint32_t>{driverId}));
			} else {
				productDrivers[productId].insert(driverId);
			}
		}
		return productDrivers;
	}

	///// Sensors

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

	uint32_t IqrfDb::insertSensor(Sensor &sensor) {
		return m_db->insert<Sensor>(sensor);
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

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

	///// Other

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
		uint8_t lastType = 0;
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

	///// Private methods /////

	void IqrfDb::initializeDatabase() {
		m_db = std::make_shared<Storage>(initializeDb(m_dbPath));
		m_db->sync_schema();
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
		initializeDatabase();
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		using namespace rapidjson;
		// path to db file
		m_dbPath = m_launchService->getDataDir() + "/DB/IqrfDb.db";
		const Document &doc = props->getAsJson();
		m_instance = Pointer("/instance").Get(doc)->GetString();
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
		TRC_FUNCTION_LEAVE("");
	}

	///// Service interfaces /////

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
