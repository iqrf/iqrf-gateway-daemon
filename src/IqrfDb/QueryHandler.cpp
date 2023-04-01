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

#include "QueryHandler.h"

QueryHandler::QueryHandler(std::shared_ptr<Storage> &db) {
	this->db = db;
}

///// General /////

std::vector<DeviceTuple> QueryHandler::getDevices() {
	std::vector<DeviceTuple> devices;
	for (auto &device : db->iterate<Device>()) {
		uint32_t productId = device.getProductId();
		Product product = db->get<Product>(productId);
		devices.push_back(std::make_tuple(device, product.getHwpid(), product.getHwpidVersion(), product.getOsBuild(), product.getOsVersion(), product.getDpaVersion()));
	}
	return devices;
}

bool QueryHandler::deviceExists(const uint8_t &address) {
	auto count = db->count<Device>(where(c(&Device::getAddress) == address));
	return count > 0;
}

std::vector<Device> QueryHandler::getDevice(const uint8_t &address) {
	return db->get_all<Device>(where(c(&Device::getAddress) == address));
}

uint32_t QueryHandler::getDeviceMid(const uint8_t &address) {
	auto mid = db->select(&Device::getMid, where(c(&Device::getAddress) == address));
	if (mid.size() == 0) {
		throw std::logic_error("Device at address " + std::to_string(address) + " does not exist.");
	}
	return mid[0];
}

std::string QueryHandler::getDeviceMetadata(const uint8_t &address) {
	auto device = db->get_all<Device>(where(c(&Device::getAddress) == address));
	if (device.size() == 0) {
		throw std::logic_error("Device at address " + std::to_string(address) + " does not exist.");
	}
	json j = {
		{"name", nullptr},
		{"location", nullptr},
		{"other", nullptr},
	};
	std::shared_ptr<std::string> val = device[0].getName();
	if (val != nullptr) {
		j["name"] = *val;
	}
	val = device[0].getLocation();
	if (val != nullptr) {
		j["location"] = *val;
	}
	val = device[0].getMetadata();
	if (val != nullptr) {
		j["other"] = *val;
	}
	return j.dump();
}

void QueryHandler::setDeviceMetadata(const uint8_t &address, const std::string &metadata) {
	auto device = db->get_all<Device>(where(c(&Device::getAddress) == address));
	if (device.size() == 0) {
		throw std::logic_error("Device at address " + std::to_string(address) + " does not exist.");
	}
	Device d = device[0];
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
	db->update(d);
}

///// Product /////

std::uint32_t QueryHandler::getCoordinatorProductId() {
	auto productId = db->select(&Product::getId,
		inner_join<Device>(on(c(&Device::getProductId) == &Product::getId)),
		where(c(&Device::getAddress) == 0)
	);
	if (productId.size() == 0) {
		return 0;
	}
	return productId[0];
}

uint32_t QueryHandler::getProductId(const uint16_t &hwpid, const uint16_t &hwpidVersion, const uint16_t &osBuild, const uint16_t &dpaVersion) {
	auto productId = db->select(&Product::getId, where(
		c(&Product::getHwpid) == hwpid
		and c(&Product::getHwpidVersion) == hwpidVersion
		and c(&Product::getOsBuild) == osBuild
		and c(&Product::getDpaVersion) == dpaVersion
	));
	if (productId.size() == 0) {
		return 0;
	}
	return productId[0];
}

std::vector<uint8_t> QueryHandler::getProductAddresses(const uint32_t &productId) {
	auto addresses = db->select(&Device::getAddress, where(c(&Device::getProductId) == productId));
	return addresses;
}

///// Drivers /////

std::map<uint32_t, std::set<uint32_t>> QueryHandler::getProductsDriversMap() {
	auto rows = db->select(columns(&Product::getId, &Driver::getId),
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

std::vector<Driver> QueryHandler::getProductDrivers(const uint32_t &productId) {
	auto productDrivers = db->get_all<ProductDriver>(where(c(&ProductDriver::getProductId) == productId));
	std::vector<Driver> drivers;
	for (auto &pd : productDrivers) {
		drivers.push_back(db->get<Driver>(pd.getDriverId()));
	}
	return drivers;
}

std::set<uint32_t> QueryHandler::getProductDriversIds(const uint32_t &productId) {
	auto productDrivers = db->get_all<ProductDriver>(where(c(&ProductDriver::getProductId) == productId));
	std::set<uint32_t> driversIds;
	for (auto &id : productDrivers) {
		driversIds.insert(id.getDriverId());
	}
	return driversIds;
}

std::string QueryHandler::getProductCustomDriver(const uint32_t &productId) {
	Product product = db->get<Product>(productId);
	std::shared_ptr<std::string> customDriver = product.getCustomDriver();
	if (customDriver == nullptr) {
		return "";
	}
	return *customDriver.get();
}

std::vector<Driver> QueryHandler::getNewestDrivers() {
	auto ids = db->select(columns(&Driver::getId, max(&Driver::getVersion)), group_by(&Driver::getPeripheralNumber));
	std::vector<Driver> drivers;
	for (auto &id : ids) {
		drivers.push_back(db->get<Driver>(std::get<0>(id)));
	}
	return drivers;
}

///// BinaryOutput /////

bool QueryHandler::boExists(const uint32_t &deviceId) {
	auto count = db->count<BinaryOutput>(where(c(&BinaryOutput::getDeviceId) == deviceId));
	return count > 0;
}

uint32_t QueryHandler::getBoId(const uint32_t &deviceId) {
	auto boId = db->select(&BinaryOutput::getId, where(c(&BinaryOutput::getDeviceId) == deviceId));
	uint32_t id = (boId.size() > 0) ? boId[0] : 0;
	return id;
}

std::map<uint8_t, uint8_t> QueryHandler::getBinaryOutputs() {
	auto rows = db->select(columns(&Device::getAddress, &BinaryOutput::getCount),
		inner_join<BinaryOutput>(on(c(&BinaryOutput::getDeviceId) == &Device::getId))
	);
	std::map<uint8_t, uint8_t> bos;
	for (auto &row : rows) {
		bos.insert(std::make_pair(std::get<0>(row), std::get<1>(row)));
	}
	return bos;
}

void QueryHandler::removeBinaryOutputs(const uint32_t &deviceId) {
	db->remove_all<BinaryOutput>(where(c(&BinaryOutput::getDeviceId) == deviceId));
}

///// DALI /////

bool QueryHandler::daliExists(const uint32_t &deviceId) {
	auto count = db->count<Dali>(where(c(&Dali::getDeviceId) == deviceId));
	return count > 0;
}

std::set<uint8_t> QueryHandler::getDalis() {
	auto rows = db->select(&Device::getAddress,
		inner_join<Dali>(on(c(&Dali::getDeviceId) == &Device::getId))
	);
	std::set<uint8_t> dalis;
	for (auto &row : rows) {
		dalis.insert(row);
	}
	return dalis;
}

void QueryHandler::removeDalis(const uint32_t &deviceId) {
	db->remove_all<Dali>(where(c(&Dali::getDeviceId) == deviceId));
}

///// Light /////

bool QueryHandler::lightExists(const uint32_t &deviceId) {
	auto count = db->count<Light>(where(c(&Light::getDeviceId) == deviceId));
	return count > 0;
}

uint32_t QueryHandler::getLightId(const uint32_t &deviceId) {
	auto lightId = db->select(&Light::getId, where(c(&Light::getDeviceId) == deviceId));
	uint32_t id = (lightId.size() > 0) ? lightId[0] : 0;
	return id;
}

std::map<uint8_t, uint8_t> QueryHandler::getLights() {
	auto rows = db->select(columns(&Device::getAddress, &Light::getCount),
		inner_join<Light>(on(c(&Light::getDeviceId) == &Device::getId))
	);
	std::map<uint8_t, uint8_t> lights;
	for (auto &row : rows) {
		lights.insert(std::make_pair(std::get<0>(row), std::get<1>(row)));
	}
	return lights;
}

void QueryHandler::removeLights(const uint32_t &deviceId) {
	db->remove_all<Light>(where(c(&Light::getDeviceId) == deviceId));
}

///// Sensor /////

bool QueryHandler::sensorTypeExists(const uint8_t &type, const std::string &name) {
	auto count = db->count<Sensor>(where(c(&Sensor::getType) == type and c(&Sensor::getName) == name));
	return count > 0;
}

uint32_t QueryHandler::getSensorId(const uint8_t &type, const std::string &name) {
	auto sensorId = db->select(&Sensor::getId, where(c(&Sensor::getType) == type and c(&Sensor::getName) == name));
	uint32_t id = (sensorId.size() > 0) ? sensorId[0] : 0;
	return id;
}

bool QueryHandler::deviceSensorExists(const uint8_t &address, const uint8_t &type, const uint8_t &index) {
	auto count = db->count<DeviceSensor>(where(c(&DeviceSensor::getAddress) == address and c(&DeviceSensor::getType) == type and c(&DeviceSensor::getIndex) == index));
	return count > 0;
}

std::map<uint8_t, std::vector<std::tuple<DeviceSensor, Sensor>>> QueryHandler::getSensors() {
	std::map<uint8_t, std::vector<std::tuple<DeviceSensor, Sensor>>> sensors;
	auto deviceSensors = db->get_all<DeviceSensor>();
	for (auto &item : deviceSensors) {
		auto device = db->get_all<Device>(where(c(&Device::getAddress) == item.getAddress()));
		if (device.size() == 0) {
			continue;
		}
		Sensor sensor = db->get<Sensor>(item.getSensorId());
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

SensorSelectMap QueryHandler::constructSensorSelectMap() {
	SensorSelectMap map;
	auto deviceSensors = db->get_all<DeviceSensor>(
		multi_order_by(
			order_by(&DeviceSensor::getAddress).asc(),
			order_by(&DeviceSensor::getType).asc(),
			order_by(&DeviceSensor::getIndex).asc()
		)
	);
	uint8_t lastAddr = 0;
	uint8_t lastType = 0;
	uint8_t idx = 0;
	for (auto &ds : deviceSensors) {
		uint8_t address = ds.getAddress();
		uint8_t type = ds.getType();
		if (lastAddr != address) {
			lastAddr = address;
			idx = 0;
		}
		if (lastType != type) {
			lastType = type;
			idx = 0;
		}
		map[type].emplace_back(std::make_tuple(address, idx++));
	}
	return map;
}

uint8_t QueryHandler::getGlobalSensorIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) {
	auto deviceSensors = db->get_all<DeviceSensor>(
		where(
			c(&DeviceSensor::getAddress) == address
			and c(&DeviceSensor::getType) == type
		),
		order_by(&DeviceSensor::getIndex).asc()
	);
	if (deviceSensors.size() <= index) {
		throw std::logic_error("Device at address " + std::to_string(address)
			+ " does not implement sensor of type " + std::to_string(type)
			+ " at index " + std::to_string(index)
		);
	}
	return deviceSensors[index].getIndex();
}

void QueryHandler::removeSensors(const uint8_t &address) {
	db->remove_all<DeviceSensor>(where(c(&DeviceSensor::getAddress) == address));
}

void QueryHandler::setSensorValue(const uint8_t &address, const uint8_t &type, const uint8_t &index, const double &value, std::shared_ptr<std::string> updated, bool frc) {
	DeviceSensor ds;
	if (frc) {
		// FRC request
		auto deviceSensors = getSensorsOfType(address, type);
		std::size_t count = deviceSensors.size();
		if (index >= count) {
			throw std::logic_error("Device at address " + std::to_string(address)
				+ " does not implement sensor of type " + std::to_string(type)
				+ " at index " + std::to_string(index)
			);
		}
		ds = deviceSensors[index];
	} else {
		// Read request
		ds = getSensorByTypeIndex(address, type, index);
	}
	ds.setValue(std::make_shared<double>(value));
	ds.setUpdated(updated);
	db->update(ds);
}

void QueryHandler::setSensorMetadata(const uint8_t &address, const uint8_t &type, const uint8_t &index, json &metadata, std::shared_ptr<std::string> updated, bool frc) {
	DeviceSensor ds;
	if (frc) {
		// FRC request
		auto deviceSensors = getSensorsOfType(address, type);
		std::size_t count = deviceSensors.size();
		if (index >= count) {
			throw std::logic_error("Device at address " + std::to_string(address)
				+ " does not implement sensor of type " + std::to_string(type)
				+ " at index " + std::to_string(index)
			);
		}
		ds = deviceSensors[index];
	} else {
		// Read request
		ds = getSensorByTypeIndex(address, type, index);
	}
	std::shared_ptr<std::string> current = ds.getMetadata();
	if (current) {
		json j = json::parse(*current.get());
		if (j.count("datablock")) {
			metadata["datablock"] = j["datablock"];
		}
	}
	current = std::make_shared<std::string>(metadata.dump());
	if (metadata.count("datablock")) {
		ds.setUpdated(updated);
	}
	ds.setMetadata(current);
	db->update(ds);
}

DeviceSensor QueryHandler::getSensorByTypeIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) {
	auto deviceSensors = db->get_all<DeviceSensor>(where(c(&DeviceSensor::getAddress) == address and c(&DeviceSensor::getType) == type and c(&DeviceSensor::getIndex) == index));
	if (deviceSensors.size() == 0) {
		throw std::logic_error("Device at address " + std::to_string(address)
			+ " does not implement sensor of type " + std::to_string(type)
			+ " at index " + std::to_string(index)
		);
	}
	return deviceSensors[0];
}

std::vector<DeviceSensor> QueryHandler::getSensorsOfType(const uint8_t &address, const uint8_t &type) {
	return db->get_all<DeviceSensor>(
		where(c(&DeviceSensor::getAddress) == address and c(&DeviceSensor::getType) == type),
		order_by(&DeviceSensor::getIndex).asc()
	);
}

std::map<uint16_t, std::set<uint8_t>> QueryHandler::getSensorDeviceHwpids(const uint8_t &type) {
	auto items = db->select(columns(&Product::getHwpid, &Device::getAddress),
		inner_join<Product>(on(c(&Product::getId) == &Device::getProductId)),
		inner_join<DeviceSensor>(on(c(&DeviceSensor::getAddress) == &Device::getAddress)),
		where(c(&DeviceSensor::getType) == type),
		group_by(&Device::getAddress)
	);
	std::map<uint16_t, std::set<uint8_t>> map;
	for (const auto &v : items) {
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

std::map<uint8_t, iqrf::embed::node::NodeMidHwpid> QueryHandler::getNodeMidHwpidMap() {
	auto records = db->select(
		columns(&Device::getAddress, &Device::getMid, &Product::getHwpid),
		inner_join<Device>(on(c(&Device::getProductId) == &Product::getId)),
		where(c(&Device::getAddress) > 0)
	);
	std::map<uint8_t, iqrf::embed::node::NodeMidHwpid> map;
	for (const auto &record : records) {
		auto addr = std::get<0>(record);
		auto mid = std::get<1>(record);
		auto hwpid = std::get<2>(record);
		map.insert(std::make_pair(addr, iqrf::embed::node::NodeMidHwpid(mid, hwpid)));
	}
	return map;
}
