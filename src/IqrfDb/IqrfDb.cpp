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
		if (m_renderService != nullptr) {
			m_renderService->clearContexts();
		}
		reloadCoordinatorDrivers();
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

	///// DB API

	Device IqrfDb::getDevice(const uint8_t &addr) {
		return this->query.getDevice(addr);
	}

	std::vector<DeviceProductTuple> IqrfDb::getDevices(std::vector<uint8_t> requestedDevices) {
		return this->query.getDevices(requestedDevices);
	}

	std::set<uint8_t> IqrfDb::getDeviceAddrs() {
		return this->query.getDeviceAddrs();
	}

	uint32_t IqrfDb::getDeviceMid(const uint8_t &address) {
		return this->query.getDeviceMid(address);
	}

	uint16_t IqrfDb::getDeviceHwpid(const uint8_t &address) {
		return this->query.getDeviceHwpid(address);
	}

	Product IqrfDb::getProductById(const uint32_t &productId) {
		return this->query.getProductById(productId);
	}

	///// DEVICE PERIPHERAL API

	bool IqrfDb::deviceImplementsPeripheral(const uint32_t &id, const int16_t peripheral) {
		auto records = m_db->select(
			&Driver::getId,
			inner_join<ProductDriver>(
				on(
					c(&ProductDriver::getDriverId) == &Driver::getId)
				),
			inner_join<Device>(
				on(c(&Device::getProductId) == &ProductDriver::getProductId)
			),
			where(
				c(&Device::getId) == id
				and c(&Driver::getPeripheralNumber) == peripheral
			)
		);
		return records.size() > 0;
	}

	///// BINARY OUTPUT API

	bool IqrfDb::binaryOutputExists(const uint32_t &deviceId) {
		auto count = m_db->count<BinaryOutput>(
			where(
				c(&BinaryOutput::getDeviceId) == deviceId
			)
		);
		return count > 0;
	}

	std::unique_ptr<BinaryOutput> IqrfDb::getBinaryOutput(const uint32_t &id) {
		return m_db->get_pointer<BinaryOutput>(id);
	}

	std::unique_ptr<BinaryOutput> IqrfDb::getBinaryOutputByDeviceId(const uint32_t &deviceId) {
		auto records = m_db->get_all<BinaryOutput>(
			where(
				c(&BinaryOutput::getDeviceId) == deviceId
			)
		);
		if (records.size() == 0) {
			return nullptr;
		}
		return std::make_unique<BinaryOutput>(records[0]);
	}

	uint32_t IqrfDb::insertBinaryOutput(BinaryOutput &binaryOutput) {
		return m_db->insert<BinaryOutput>(binaryOutput);
	}

	void IqrfDb::updateBinaryOutput(BinaryOutput &binaryOutput) {
		m_db->update<BinaryOutput>(binaryOutput);
	}

	void IqrfDb::removeBinaryOutput(const uint32_t &id) {
		m_db->remove<BinaryOutput>(id);
	}

	void IqrfDb::removeBinaryOutputByDeviceId(const uint32_t &deviceId) {
		m_db->remove_all<BinaryOutput>(
			where(
				c(&BinaryOutput::getDeviceId) == deviceId
			)
		);
	}

	std::set<uint8_t> IqrfDb::getBinaryOutputAddresses() {
		auto records = m_db->select(
			&Device::getAddress,
			inner_join<BinaryOutput>(
				on(
					c(&BinaryOutput::getDeviceId) == &Device::getId
				)
			)
		);
		return std::set<uint8_t>(records.begin(), records.end());
	}

	std::map<uint8_t, uint8_t> IqrfDb::getBinaryOutputCountMap() {
		auto records = m_db->select(
			columns(
				&Device::getAddress,
				&BinaryOutput::getCount
			),
			inner_join<BinaryOutput>(
				on(
					c(&BinaryOutput::getDeviceId) == &Device::getId
				)
			)
		);
		std::map<uint8_t, uint8_t> map;
		for (auto const &record : records) {
			map.insert(
				std::make_pair(
					std::get<0>(record),
					std::get<1>(record)
				)
			);
		}
		return map;
	}

	///// LIGHT API


	bool IqrfDb::lightExists(const uint32_t &deviceId) {
		auto count = m_db->count<Light>(
			where(
				c(&Light::getDeviceId) == deviceId
			)
		);
		return count > 0;
	}

	std::unique_ptr<Light> IqrfDb::getLight(const uint32_t &id) {
		return m_db->get_pointer<Light>(id);
	}

	std::unique_ptr<Light> IqrfDb::getLightByDeviceId(const uint32_t &deviceId) {
		auto records = m_db->get_all<Light>(
			where(
				c(&Light::getDeviceId) == deviceId
			)
		);
		if (records.size() == 0) {
			return nullptr;
		}
		return std::make_unique<Light>(records[0]);
	}

	uint32_t IqrfDb::insertLight(Light &light) {
		return m_db->insert<Light>(light);
	}

	void IqrfDb::updateLight(Light &light) {
		m_db->update<Light>(light);
	}

	void IqrfDb::removeLight(const uint32_t &id) {
		m_db->remove<Light>(id);
	}

	void IqrfDb::removeLightByDeviceId(const uint32_t &deviceId) {
		m_db->remove_all<Light>(
			where(
				c(&Light::getDeviceId) == deviceId
			)
		);
	}

	std::set<uint8_t> IqrfDb::getLightAddresses() {
		auto records = m_db->select(
			&Device::getAddress,
			inner_join<Light>(
				on(
					c(&Light::getDeviceId) == &Device::getId
				)
			)
		);
		return std::set<uint8_t>(records.begin(), records.end());
	}

	///// SENSOR API

	std::unique_ptr<Sensor> IqrfDb::getSensorByAddrIndexType(const uint8_t &deviceAddress, const uint8_t &index, const uint8_t &type) {
		auto records = m_db->select(
			&DeviceSensor::getSensorId,
			where(
				c(&DeviceSensor::getAddress) == deviceAddress
				and c(&DeviceSensor::getGlobalIndex) == index
				and c(&DeviceSensor::getType) == type
			)
		);
		if (records.size() == 0) {
			return nullptr;
		}
		return m_db->get_pointer<Sensor>(records[0]);
	}

	bool IqrfDb::hasSensors(const uint8_t &deviceAddress) {
		return this->query.hasSensors(deviceAddress);
	}

	std::map<uint8_t, Sensor> IqrfDb::getDeviceSensorsByAddress(const uint8_t &deviceAddress) {
		return this->query.getDeviceSensorsByAddress(deviceAddress);
	}

	std::map<uint8_t, uint32_t> IqrfDb::getDeviceSensorIndexIdMap(const uint8_t &deviceAddress) {
		return this->query.getDeviceSensorIndexIdMap(deviceAddress);
	}

	std::map<uint8_t, std::vector<std::tuple<DeviceSensor, Sensor>>> IqrfDb::getSensors() {
		return this->query.getSensors();
	}

	SensorSelectMap IqrfDb::constructSensorSelectMap() {
		return this->query.constructSensorSelectMap();
	}

	uint8_t IqrfDb::getGlobalSensorIndex(const uint8_t &address, const uint8_t &type, const uint8_t &index) {
		return this->query.getGlobalSensorIndex(address, type, index);
	}

	void IqrfDb::setSensorValue(const uint8_t &address, const uint8_t &type, const uint8_t &index, const double &value, std::shared_ptr<std::string> updated) {
		this->query.setSensorValue(address, type, index, value, updated);
	}

	std::shared_ptr<std::string> IqrfDb::getDeviceMetadata(const uint8_t &address) {
		return this->query.getDeviceMetadata(address);
	}

	rapidjson::Document IqrfDb::getDeviceMetadataDoc(const uint8_t &address) {
		auto metadata = this->query.getDeviceMetadata(address);
		rapidjson::Document doc;
		if (metadata) {
			doc.Parse(metadata.get()->c_str());
			if (doc.HasParseError()) {
				THROW_EXC_TRC_WAR(std::logic_error, "Invalid json syntax in metadata: " << doc.GetParseError() << ", " << doc.GetErrorOffset());
			}
		}
		return doc;
	}

	void IqrfDb::setDeviceMetadata(const uint8_t &address, std::shared_ptr<std::string> metadata) {
		this->query.setDeviceMetadata(address, metadata);
	}

	std::map<uint16_t, std::set<uint8_t>> IqrfDb::getSensorDeviceHwpids(const uint8_t &type) {
		return this->query.getSensorDeviceHwpids(type);
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

	void IqrfDb::updateSensorValues(const std::map<uint8_t, std::vector<sensor::item::Sensor>> &devices) {
		TRC_FUNCTION_ENTER("");
		std::shared_ptr<std::string> timestamp = IqrfDbAux::getCurrentTimestamp();
		for (auto &device : devices) {
			const uint8_t addr = device.first;
			if (!this->query.deviceExists(addr)) {
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
						this->query.setSensorMetadata(addr, sensor.getType(), sensor.getIdx(), block, timestamp);
					} else {
						double val;
						if (sensor.hasBreakdown()) {
							val = sensor.getBreakdownValue();
						} else {
							val = sensor.getValue();
						}
						this->query.setSensorValue(addr, sensor.getType(), sensor.getIdx(), val, timestamp);
					}
				} catch (const std::logic_error &e) {
					TRC_WARNING(e.what());
				}
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::updateSensorValues(const uint8_t &address, const std::string &sensors) {
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
					this->query.setSensorMetadata(address, item["type"], i, block, timestamp);
					continue;
				}
				val = item["value"];
				if (item["type"] == 129 || item["type"] == 160) {
					if (!item["breakdown"][0]["value"].is_null()) {
						val = item["breakdown"][0]["value"];
					}
				}
				this->query.setSensorValue(address, item["type"], i, val, timestamp);
			} catch (const std::logic_error &e) {
				TRC_WARNING(e.what());
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::updateSensorValues(const uint8_t &type, const uint8_t &index, const std::set<uint8_t> &selectedNodes, const std::string &sensors) {
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
					this->query.setSensorMetadata(i, type, index, block, timestamp, true);
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
					this->query.setSensorValue(i, type, index, val, timestamp, true);
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
					this->query.setSensorMetadata(i, type, index, block, timestamp, true);
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
					this->query.setSensorValue(i, type, index, val, timestamp, true);
				} catch (const std::logic_error &e) {
					TRC_WARNING(e.what());
				}
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	std::map<uint8_t, embed::node::NodeMidHwpid> IqrfDb::getNodeMidHwpidMap() {
		return this->query.getNodeMidHwpidMap();
	}

	///// Private methods /////

	void IqrfDb::initializeDatabase() {
		migrateDatabase();
		m_db = std::make_shared<Storage>(initializeDb(m_dbPath));
		try {
			auto res = m_db->sync_schema(true);
			this->query = QueryHandler(m_db);
		} catch (const std::exception &e) {
			THROW_EXC_TRC_WAR(std::logic_error, "[IqrfDB] Sync schema failed: " << e.what());
		}
	}

	void IqrfDb::migrateDatabase() {
		// find all migrations
		std::string migrationDir = m_dbDirPath + "migrations/";
		std::vector<std::string> migrations;
		for (const auto &file : std::filesystem::directory_iterator(migrationDir)) {
			if (file.is_regular_file()) {
				migrations.push_back(file.path().stem());
			}
		}
		std::sort(migrations.begin(), migrations.end());
		// determine which migrations need to be executed
		bool exists = std::filesystem::exists(m_dbPath);
		SQLite::Database db(m_dbPath, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
		std::vector<std::string> migrationsToExecute;
		if (!exists) {
			// DB does not exist, execute all migrations
			migrationsToExecute = migrations;
		} else {
			// DB exists, get executed migrations
			std::set<std::string> executedMigrations;
			SQLite::Statement query(db, "SELECT m.version FROM migrations as m");
			while (query.executeStep()) {
				auto version = query.getColumn(0).getString();
				executedMigrations.insert(version);
			}
			// determine executed migrations and migrations to execute
			for (auto &migration : migrations) {
				if (executedMigrations.count(migration) == 0) {
					migrationsToExecute.push_back(migration);
				}
			}
		}
		// execute missing migrations
		for (const auto &migration : migrationsToExecute) {
			executeMigration(db, migrationDir + migration + ".sql");
		}
	}

	void IqrfDb::executeMigration(SQLite::Database &db, const std::string &migration) {
		std::vector<std::string> statements;
		// try to access migration file
		std::ifstream migrationFile(migration);
		if (!migrationFile.is_open()) {
			THROW_EXC_TRC_WAR(std::logic_error, "Unable to read migration file: " << migration);
		}
		std::string line;
		std::stringstream statementStream;
		// remove comments and empty lines
		while (std::getline(migrationFile, line)) {
			if (line.empty() || line.rfind("--", 0) == 0) {
				continue;
			}
			statementStream << line;
		}
		// split into separate statements
		while (std::getline(statementStream, line, ';')) {
			statements.push_back(line);
		}
		// check for empty file
		if (statements.size() == 0) {
			THROW_EXC_TRC_WAR(std::logic_error, "Empty migration file: " << migration);
		}
		try {
			// execute migration statements
			for (auto &statement : statements) {
				db.exec(statement);
			}
		} catch (const std::exception &e) {
			THROW_EXC_TRC_WAR(std::logic_error, e.what());
		}
	}

	void IqrfDb::updateDbProductNames() {
		TRC_FUNCTION_ENTER("");
		for (auto dbProduct : m_db->iterate<Product>()) {
			if (dbProduct.getName() != nullptr) {
				continue;
			}
			auto cacheProduct = m_cacheService->getProduct(dbProduct.getHwpid());
			if (!cacheProduct) {
				continue;
			}
			dbProduct.setName(std::make_shared<std::string>(cacheProduct->m_name));
			m_db->update(dbProduct);
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::updateDbDrivers() {
		TRC_FUNCTION_ENTER("");
		auto dbDrivers = m_db->get_all<Driver>();
		for (auto &dbDriver : dbDrivers) {
			auto driver = m_cacheService->getDriver(dbDriver.getPeripheralNumber(), dbDriver.getVersion());
			if (driver == nullptr) {
				continue;
			}
			std::string driverHash = generateDriverHash(*driver->getDriver());
			if (driverHash == dbDriver.getDriverHash()) {
				continue;
			}
			TRC_INFORMATION("[IqrfDb] Updating code of driver per " << std::to_string(dbDriver.getPeripheralNumber()) << ", version " << std::to_string(dbDriver.getVersion()));
			dbDriver.setDriver(*driver->getDriver());
			dbDriver.setDriverHash(driverHash);
			m_db->update(dbDriver);
		}
		TRC_FUNCTION_LEAVE("");
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

					sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::Devices));
					enumerateDevices();
					sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::DevicesDone));

					sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::Products));
					productPackageEnumeration();
					updateDatabaseProducts();
					loadProductDrivers();
					sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::ProductsDone));

					if (m_params.standards || m_params.reenumerate) {
						sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::Standards));
						standardEnumeration();
						sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::StandardsDone));
					}
					sendEnumerationResponse(EnumerationProgress(EnumerationProgress::Steps::Finish));
					resetExclusiveAccess();
					m_enumRepeat = false;
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

		auto dbDevices = m_db->get_all<Device>();

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

	std::set<int> IqrfDb::getEmbeddedStandardPeripherals(const uint8_t &addr) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		try {
			DpaMessage perEnumRequest;
			DpaMessage::DpaPacket_t perEnumPacket;
			perEnumPacket.DpaRequestPacket_t.NADR = addr;
			perEnumPacket.DpaRequestPacket_t.PNUM = PNUM_ENUMERATION;
			perEnumPacket.DpaRequestPacket_t.PCMD = 0x3F;
			perEnumPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			perEnumRequest.DataToBuffer(perEnumPacket.Buffer, sizeof(TDpaIFaceHeader));
			m_dpaService->executeDpaTransactionRepeat(perEnumRequest, result, 1);
		} catch (const std::exception &e) {
			THROW_EXC(std::logic_error, e.what());
		}
		auto response = result->getResponse();
		auto pers = HexStringConversion::bitmapToIndexes(response.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer.EmbeddedPers, 0, 3, 0);
		if (response.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer.UserPerNr > 0) {
			auto userPers = HexStringConversion::bitmapToIndexes(response.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer.UserPer, 0, 11, 0x20);
			for (auto per : userPers) {
				pers.insert(per);
			}
		}
		pers.insert(-1);
		pers.insert(255);
		TRC_FUNCTION_LEAVE("");
		return pers;
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
			if ((hwpid & 0x000F) == 0xF) {
				this->enumerateNoncertifiedProduct(addr);
				continue;
			}
			uint16_t hwpidVersion = product->getHwpidVersion();
			uint16_t osBuild = product->getOsBuild();
			uint16_t dpaVersion = product->getDpaVersion();
			std::shared_ptr<IJsCacheService::Product> cacheProduct = m_cacheService->getProduct(hwpid);
			if (cacheProduct != nullptr) {
				product->setName(std::make_shared<std::string>(cacheProduct->m_name));
			}
			std::shared_ptr<IJsCacheService::Package> package = m_cacheService->getPackage(hwpid, hwpidVersion, osBuild, dpaVersion);
			if (package == nullptr) {
				// try to find db product
				uint32_t productId = this->query.getProductId(hwpid, hwpidVersion, osBuild, dpaVersion);
				if (productId > 0) {
					// found in db, enumerate from existing record
					Product dbProduct = m_db->get<Product>(productId);
					product->setHandlerUrl(dbProduct.getHandlerUrl());
					product->setHandlerHash(dbProduct.getHandlerHash());
					product->setCustomDriver(dbProduct.getCustomDriver());
					product->setPackageId(dbProduct.getPackageId());
					product->setName(dbProduct.getName());
					std::set<uint32_t> driverIds = this->query.getProductDriversIds(productId);
					for (auto id : driverIds) {
						product->drivers.insert(id);
					}
					continue;
				}
				this->enumerateNoncertifiedProduct(addr);
				continue;
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
			if (package->m_driver.length() != 0) {
				product->setCustomDriver(std::make_shared<std::string>(package->m_driver));
			}
			product->setPackageId(std::make_shared<uint32_t>(package->m_packageId));
			for (auto &item : package->m_stdDriverVect) {
				int16_t per = item.getId();
				double version = item.getVersion();
				auto dbDriver = m_db->select(&Driver::getId, where(c(&Driver::getPeripheralNumber) == per and c(&Driver::getVersion) == version));
				if (dbDriver.size() == 0) {
					Driver driver(item.getName(), per, version, item.getVersionFlags(), *item.getDriver(), generateDriverHash(*item.getDriver()));
					uint32_t driverId = m_db->insert(driver);
					product->drivers.insert(driverId);
				} else {
					product->drivers.insert(dbDriver[0]);
				}
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::enumerateNoncertifiedProduct(const uint8_t &addr) {
		auto &product = m_deviceProductMap[addr];
		uint32_t dbProductId = this->query.getProductIdNoncertified(
			product->getHwpid(),
			product->getHwpidVersion(),
			product->getOsBuild(),
			product->getDpaVersion()
		);
		if (dbProductId > 0) {
			std::set<uint32_t> driverIds = this->query.getProductDriversIds(dbProductId);
			for (auto id : driverIds) {
				product->drivers.insert(id);
			}
			return;
		}
		auto pers = getEmbeddedStandardPeripherals(addr);
		if ((pers.count(PERIPHERAL_SENSOR) == 1 || pers.count(PERIPHERAL_LIGHT)) && pers.count(PNUM_FRC) == 0) {
			pers.insert(PNUM_FRC);
		}
		for (auto &per : pers) {
			auto candidate = m_cacheService->getLatestDriver(per);
			if (candidate == nullptr) {
				continue;
			}
			double version = candidate->getVersion();
			auto dbDriver = m_db->select(&Driver::getId, where(c(&Driver::getPeripheralNumber) == per and c(&Driver::getVersion) == version));
			if (dbDriver.size() == 0) {
				Driver driver(candidate->getName(), per, version, candidate->getVersionFlags(), *candidate->getDriver(), generateDriverHash(*candidate->getDriver()));
				uint32_t driverId = m_db->insert(driver);
				product->drivers.insert(driverId);
			} else {
				product->drivers.insert(dbDriver[0]);
			}
		}
	}

	void IqrfDb::updateDatabaseProducts() {
		TRC_FUNCTION_ENTER("");
		using namespace sqlite_orm;
		m_db->begin_transaction();
		for (auto &deleteId : m_toDelete) {
			m_db->remove<Device>(deleteId);
		}
		uint32_t productId = -1;
		for (auto &addr : m_toEnumerate) {
			ProductPtr &product = m_deviceProductMap[addr];
			try {
				// create a new product or update existing
				auto dbProduct = m_db->select(&Product::getId,
					where(
						c(&Product::getHwpid) == product->getHwpid() and
						c(&Product::getHwpidVersion) == product->getHwpidVersion() and
						c(&Product::getOsBuild) == product->getOsBuild() and
						c(&Product::getDpaVersion) == product->getDpaVersion()
					)
				);

				// store new product or use existing
				if (dbProduct.size() == 0) {
					productId = m_db->insert(*product.get());
				} else {
					productId = dbProduct[0];
				}

				// update product drivers, reuse dbProductDrivers to delete old unused drivers by erasing intersecting elements
				auto dbProductDrivers = this->query.getProductDriversIds(productId);
				if (dbProductDrivers != product->drivers) {
					for (auto &driverId : product->drivers) {
						if (dbProductDrivers.count(driverId)) {
							// product driver in db, do not add and remove from old db set
							dbProductDrivers.erase(driverId);
							continue;
						} else {
							// add new product driver
							ProductDriver productDriver(productId, driverId);
							m_db->replace(productDriver);
						}
					}
					// remove old unused product drivers
					for (auto &driverId : dbProductDrivers) {
						// remove old set
						m_db->remove_all<ProductDriver>(
							where(
								c(&ProductDriver::getProductId) == productId and
								c(&ProductDriver::getDriverId) == driverId
							)
						);
					}
				}


				// create a new device or update existing
				bool discovered = m_discovered.find(addr) != m_discovered.end();
				uint32_t mid = m_mids[addr];
				uint8_t vrn = discovered ? m_vrns[addr] : 0;
				uint8_t zone = discovered ? m_zones[addr] : 0;
				std::shared_ptr<uint8_t> parent = discovered ? std::make_shared<uint8_t>(m_parents[addr]) : nullptr;
				if (!this->query.deviceExists(addr)) {
					// create new
					Device device(addr, discovered, mid, vrn, zone, parent);
					device.setProductId(productId);
					m_db->insert(device);
				} else {
					// update existing
					Device device = this->query.getDevice(addr);
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
					m_db->update(device);
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
		for (auto &device : m_db->iterate<Device>()) {
			if (!device.isEnumerated() || m_toEnumerate.count(device.getAddress())) {
				devices.insert(std::make_pair(device.getId(), device.getAddress()));
			}
		}

		for (auto &[deviceId, address] : devices) {
			// begin transaction
			m_db->begin_transaction();
			try {
				if (this->deviceImplementsPeripheral(deviceId, PERIPHERAL_BINOUT)) {
					binoutEnumeration(deviceId, address);
				} else {
					if (this->binaryOutputExists(deviceId)) {
						this->removeBinaryOutputByDeviceId(deviceId);
					}
				}
				if (this->deviceImplementsPeripheral(deviceId, PERIPHERAL_LIGHT)) {
					lightEnumeration(deviceId);
				} else {
					if (this->lightExists(deviceId)) {
						this->removeLightByDeviceId(deviceId);
					}
				}
				if (this->deviceImplementsPeripheral(deviceId, PERIPHERAL_SENSOR)) {
					sensorEnumeration(address);
				} else {
					this->query.removeSensors(address);
				}
				// set as enumerated
				auto dbDevice = m_db->get<Device>(deviceId);
				dbDevice.setEnumerated(true);
				m_db->update<Device>(dbDevice);
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

		auto dbBinaryOutput = this->getBinaryOutputByDeviceId(deviceId);
		if (dbBinaryOutput == nullptr) {
			BinaryOutput newBinaryOutput(deviceId, count);
			this->insertBinaryOutput(newBinaryOutput);
		} else if (dbBinaryOutput->getCount() != count) {
			dbBinaryOutput->setCount(count);
			this->updateBinaryOutput(*dbBinaryOutput);
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::lightEnumeration(const uint32_t &deviceId) {
		TRC_FUNCTION_ENTER("");
		using namespace sqlite_orm;

		auto dbLight = this->getLightByDeviceId(deviceId);
		if (dbLight == nullptr) {
			Light newLight(deviceId);
			this->insertLight(newLight);
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfDb::sensorEnumeration(const uint8_t &address) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		sensor::jsdriver::Enumerate sensorEnum(m_renderService, address);
		m_dpaService->executeDpaTransactionRepeat(sensorEnum.getRequest(), result, 1);
		sensorEnum.processDpaTransactionResult(std::move(result));

		auto oldSensors = this->query.getDeviceSensorIndexIdMap(address);
		auto &sensors = sensorEnum.getSensors();

		uint8_t cnt[255] = {0};

		for (auto &item : sensors) {
			uint32_t sensorId;
			uint8_t type = item->getType();
			bool breakdown = item->hasBreakdown();
			std::string name = breakdown ? item->getBreakdownName() : item->getName();
			bool exists = this->query.sensorTypeExists(type, name);
			if (exists) {
				sensorId = this->query.getSensorId(type, name);
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
				sensorId = m_db->insert(sensor);
			}

			// store device and sensor
			const uint8_t index = item->getIdx();

			auto candidate = oldSensors.find(index);
			if (candidate == oldSensors.end() || (candidate != oldSensors.end() && candidate->second != sensorId)) {
				this->query.removeDeviceSensor(address, index);
				m_db->replace(DeviceSensor(address, type, index, cnt[type], sensorId, nullptr));
				cnt[type] += 1;
			}
			oldSensors.erase(index);
		}

		if (oldSensors.size() > 0) {
			std::vector<uint8_t> indexesToDelete;
			indexesToDelete.reserve(oldSensors.size());
			for (const auto &[key, value] : oldSensors) {
				indexesToDelete.push_back(key);
			}
			this->query.removeDeviceSensors(address, indexesToDelete);
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

		for (auto &[driverId, versionMap] : drivers) {
			double driverVersion = 0;
			driversToLoad.insert(driverId);

			if (versionMap.size() > 0) {
				// use latest
				driverVersion = versionMap.rbegin()->first;
			} else {
				TRC_WARNING("No driver version found for driver ID: " << driverId);
			}

			std::shared_ptr<IJsCacheService::StdDriver> cacheDriver = m_cacheService->getDriver(driverId, driverVersion);
			if (cacheDriver != nullptr) {
				ss << *cacheDriver->getDriver();
				TRC_DEBUG("[C] Loading driver ID " << driverId << ", version " << driverVersion);
			} else {
				TRC_WARNING("[C] No driver found in cache for ID: " << driverId << ", version: " << driverVersion);
			}
			if (driverId == PERIPHERAL_LIGHT) {
				if (driverVersion > 0) {
					cacheDriver = m_cacheService->getDriver(driverId, 0);
					if (cacheDriver != nullptr) {
						ss << *cacheDriver->getDriver();
						TRC_INFORMATION("Loading deprecated DALI driver for LIGHT backward compatibility.");
					}
				} else {
					cacheDriver = m_cacheService->getDriver(driverId, 1);
					if (cacheDriver != nullptr) {
						ss << *cacheDriver->getDriver();
						TRC_INFORMATION("Loading LIGHT driver for DALI forward compatibility.")
					}
				}
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

			std::map<uint32_t, std::set<uint32_t>> productsDriversMap = this->query.getProductsDriversMap();
			std::set<uint32_t> productsToLoad;

			uint32_t coordinatorProductId = this->query.getCoordinatorProductId();

			auto deviceAddrProductIdMap = this->query.getDeviceProductIdMap();
			for (auto &[addr, productId] : deviceAddrProductIdMap) {
				if (productsToLoad.count(productId)) {
					continue;
				}
				auto loadedProductId = m_renderService->getDeviceAddrProductId(addr);
				if (loadedProductId == nullptr || *loadedProductId.get() != productId) {
					productsToLoad.insert(productId);
					continue;
				}
				auto loadedDrivers = m_renderService->getDriverIdSet(productId);
				auto productDriverRecord = productsDriversMap.find(productId);
				if (productDriverRecord == productsDriversMap.end()) {
					continue;
				}
				auto productDriverSet = productDriverRecord->second;
				if (productDriverSet != loadedDrivers) {
					productsToLoad.insert(productId);
				}
			}

			for (uint32_t productId : productsToLoad) {
				std::string customDriver = this->query.getProductCustomDriver(productId);
				std::vector<Driver> drivers = this->query.getProductDrivers(productId);

				if (productId == coordinatorProductId) { // ensure standard FRC backwards compatibility
					drivers = this->query.getNewestDrivers();
				}

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

				std::vector<uint8_t> addresses = this->query.getProductAddresses(productId);

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
		} catch (std::exception &e) {
			CATCH_EXC_TRC_WAR(std::exception, e, "Failed to load drivers: " << e.what());
		}

		TRC_FUNCTION_LEAVE("");
	}

	///// Auxiliary functions /////

	std::string IqrfDb::generateDriverHash(const std::string &driver) {
		EVP_MD_CTX* ctx = EVP_MD_CTX_new();

		if (ctx == nullptr) {
			THROW_EXC_TRC_WAR(std::logic_error, "Failed to generate driver hash, context not created.");
		}

		if (!EVP_DigestInit_ex(ctx, EVP_sha256(), NULL)) {
			EVP_MD_CTX_free(ctx);
			THROW_EXC_TRC_WAR(std::logic_error, "Failed to generate driver hash, digest initialization failed.");
		}

		if (!EVP_DigestUpdate(ctx, driver.c_str(), driver.length())) {
			EVP_MD_CTX_free(ctx);
			THROW_EXC_TRC_WAR(std::logic_error, "Failed to generate driver hash, digest update failed.");
		}

		unsigned char digest[EVP_MAX_MD_SIZE];
		unsigned int hashLen = 0;

		if (!EVP_DigestFinal_ex(ctx, digest, &hashLen)) {
			EVP_MD_CTX_free(ctx);
			THROW_EXC_TRC_WAR(std::logic_error, "Faield to generate driver hash, digest final failed.");
		}

		std::ostringstream oss;
		for (unsigned int i = 0; i < hashLen; ++i) {
			oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
		}

		EVP_MD_CTX_free(ctx);
		return oss.str();
	}

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
			updateDbDrivers();
			reloadDrivers();
		});
		m_dpaService->registerAnyMessageHandler(m_instance, [&](const DpaMessage &msg) {
			analyzeDpaMessage(msg);
		});
		initializeDatabase();
		updateDbProductNames();
		updateDbDrivers();
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
		//
		m_dbDirPath = m_launchService->getDataDir() + "/DB/";
		// path to db file
		m_dbPath = m_dbDirPath + "IqrfDb.db";
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
