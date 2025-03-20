/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "QueryHandler.h"

QueryHandler::QueryHandler(std::shared_ptr<Storage> &db) {
	this->db = db;
}

///// General /////

std::vector<DeviceProductTuple> QueryHandler::getDevices(std::vector<uint8_t> requestedDevices) {
	std::vector<DeviceProductTuple> devices;
	std::vector<Device> dbDevices;
	if (requestedDevices.size() == 0) {
		dbDevices = db->get_all<Device>(order_by(&Device::getAddress));
	} else {
		dbDevices = db->get_all<Device>(where(in(&Device::getAddress, requestedDevices)), order_by(&Device::getAddress));
	}
	for (auto &device : dbDevices) {
		uint32_t productId = device.getProductId();
		Product product = db->get<Product>(productId);
		devices.push_back(std::make_tuple(device, product));
	}
	return devices;
}

std::set<uint8_t> QueryHandler::getDeviceAddrs() {
	std::set<uint8_t> addrs;
	auto devices = db->get_all<Device>();
	for (const auto &device : devices) {
		addrs.insert(device.getAddress());
	}
	return addrs;
}

bool QueryHandler::deviceExists(const uint8_t &address) {
	auto count = db->count<Device>(where(c(&Device::getAddress) == address));
	return count > 0;
}

Device QueryHandler::getDevice(const uint8_t &address) {
	auto devices = db->get_all<Device>(where(c(&Device::getAddress) == address));
	if (devices.size() == 0) {
		throw std::logic_error("Device at address " + std::to_string(address) + " does not exist.");
	}
	return devices[0];
}

uint16_t QueryHandler::getDeviceHwpid(const uint8_t &address) {
	auto hwpid = db->select(&Product::getHwpid,
		inner_join<Product>(on(c(&Product::getId) == &Device::getProductId)),
		where(c(&Device::getAddress) == address)
	);
	if (hwpid.size() == 0) {
		throw std::logic_error("Device at address " + std::to_string(address) + " does not exist.");
	}
	return hwpid[0];
}

uint32_t QueryHandler::getDeviceMid(const uint8_t &address) {
	auto mid = db->select(&Device::getMid, where(c(&Device::getAddress) == address));
	if (mid.size() == 0) {
		throw std::logic_error("Device at address " + std::to_string(address) + " does not exist.");
	}
	return mid[0];
}

std::shared_ptr<std::string> QueryHandler::getDeviceMetadata(const uint8_t &address) {
	auto device = db->get_all<Device>(where(c(&Device::getAddress) == address));
	if (device.size() == 0) {
		throw std::logic_error("Device at address " + std::to_string(address) + " does not exist.");
	}
	return device[0].getMetadata();
}

void QueryHandler::setDeviceMetadata(const uint8_t &address, std::shared_ptr<std::string> metadata) {
	auto device = db->get_all<Device>(where(c(&Device::getAddress) == address));
	if (device.size() == 0) {
		throw std::logic_error("Device at address " + std::to_string(address) + " does not exist.");
	}
	Device d = device[0];
	d.setMetadata(metadata);
	db->update(d);
}

std::map<uint8_t, uint32_t> QueryHandler::getDeviceProductIdMap() {
	std::map<uint8_t, uint32_t> map;
	auto records = db->select(columns(&Device::getAddress, &Device::getProductId));
	for (auto &record : records) {
		map.insert_or_assign(std::get<0>(record), std::get<1>(record));
	}
	return map;
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
		and c(&Product::getPackageId) != nullptr
	));
	if (productId.size() == 0) {
		return 0;
	}
	return productId[0];
}

uint32_t QueryHandler::getProductIdNoncertified(const uint16_t &hwpid, const uint16_t &hwpidVersion, const uint16_t &osBuild, const uint16_t &dpaVersion) {
	auto productId = db->select(&Product::getId, where(
		c(&Product::getHwpid) == hwpid
		and c(&Product::getHwpidVersion) == hwpidVersion
		and c(&Product::getOsBuild) == osBuild
		and c(&Product::getDpaVersion) == dpaVersion
		and c(&Product::getPackageId) == nullptr
	));
	if (productId.size() == 0) {
		return 0;
	}
	return productId[0];
}

Product QueryHandler::getProductById(const uint32_t &productId) {
	auto products = db->get_all<Product>(where(c(&Product::getId) == productId));
	if (products.size() == 0) {
		throw std::logic_error("Product record with ID " + std::to_string(productId) + " does not exist.");
	}
	return products[0];
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

///// Sensor /////

bool QueryHandler::hasSensors(const uint8_t &deviceAddress) {
	auto count = db->count<DeviceSensor>(where(c(&DeviceSensor::getAddress) == deviceAddress));
	return count > 0;
}

std::map<uint8_t, Sensor> QueryHandler::getDeviceSensorsByAddress(const uint8_t &deviceAddress) {
	auto deviceSensors = db->get_all<DeviceSensor>(where(c(&DeviceSensor::getAddress) == deviceAddress));
	std::map<uint8_t, Sensor> sensors;
	for (auto ds : deviceSensors) {
		auto sensor = db->get<Sensor>(ds.getSensorId());
		sensors.insert(std::make_pair(ds.getGlobalIndex(), sensor));
	}
	return sensors;
}

std::map<uint8_t, uint32_t> QueryHandler::getDeviceSensorIndexIdMap(const uint8_t &deviceAddress) {
	auto deviceSensors = db->get_all<DeviceSensor>(where(c(&DeviceSensor::getAddress) == deviceAddress));
	std::map<uint8_t, uint32_t> map;
	for (auto ds : deviceSensors) {
		map.insert_or_assign(ds.getGlobalIndex(), ds.getSensorId());
	}
	return map;
}

bool QueryHandler::sensorTypeExists(const uint8_t &type, const std::string &name) {
	auto count = db->count<Sensor>(where(c(&Sensor::getType) == type and c(&Sensor::getName) == name));
	return count > 0;
}

uint32_t QueryHandler::getSensorId(const uint8_t &type, const std::string &name) {
	auto sensorId = db->select(&Sensor::getId, where(c(&Sensor::getType) == type and c(&Sensor::getName) == name));
	uint32_t id = (sensorId.size() > 0) ? sensorId[0] : 0;
	return id;
}

bool QueryHandler::deviceSensorExists(const uint8_t &address, const uint32_t &sensorId, const uint8_t &index) {
	auto count = db->count<DeviceSensor>(
		where(
			c(&DeviceSensor::getAddress) == address and
			c(&DeviceSensor::getSensorId) == sensorId and
			c(&DeviceSensor::getGlobalIndex) == index
		)
	);
	return count > 0;
}

std::map<uint8_t, std::vector<std::tuple<DeviceSensor, Sensor>>> QueryHandler::getSensors() {
	std::map<uint8_t, std::vector<std::tuple<DeviceSensor, Sensor>>> sensors;
	auto deviceSensors = db->get_all<DeviceSensor>(multi_order_by(
		order_by(&DeviceSensor::getAddress).asc(),
		order_by(&DeviceSensor::getGlobalIndex).asc()
	));
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

uint8_t QueryHandler::getGlobalSensorIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) {
	auto deviceSensors = db->get_all<DeviceSensor>(
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

void QueryHandler::removeSensors(const uint8_t &address) {
	db->remove_all<DeviceSensor>(where(c(&DeviceSensor::getAddress) == address));
}

void QueryHandler::removeDeviceSensor(const uint8_t &address, const uint8_t &index) {
	db->remove_all<DeviceSensor>(where(c(&DeviceSensor::getAddress) == address) and c(&DeviceSensor::getGlobalIndex) == index);
}

void QueryHandler::removeDeviceSensors(const uint8_t &address, const std::vector<uint8_t> &indexes) {
	db->remove_all<DeviceSensor>(where(c(&DeviceSensor::getAddress) == address and in(&DeviceSensor::getGlobalIndex, indexes)));
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
	auto deviceSensors = db->get_all<DeviceSensor>(where(c(&DeviceSensor::getAddress) == address and c(&DeviceSensor::getType) == type and c(&DeviceSensor::getGlobalIndex) == index));
	if (deviceSensors.size() == 0) {
		throw std::logic_error("Device at address " + std::to_string(address)
			+ " does not implement sensor of type " + std::to_string(type)
			+ " at index " + std::to_string(index)
		);
	}
	return deviceSensors[0];
}

DeviceSensor QueryHandler::getDeviceSensorByIndex(const uint8_t &address, const uint8_t &index) {
	auto deviceSensors = db->get_all<DeviceSensor>(where(c(&DeviceSensor::getAddress) == address and c(&DeviceSensor::getGlobalIndex) == index));
	if (deviceSensors.size() == 0) {
		throw std::logic_error("Device at address " + std::to_string(address) + " does not implement at index " + std::to_string(index));
	}
	return deviceSensors[0];
}

std::vector<DeviceSensor> QueryHandler::getSensorsOfType(const uint8_t &address, const uint8_t &type) {
	return db->get_all<DeviceSensor>(
		where(c(&DeviceSensor::getAddress) == address and c(&DeviceSensor::getType) == type),
		order_by(&DeviceSensor::getGlobalIndex).asc()
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
