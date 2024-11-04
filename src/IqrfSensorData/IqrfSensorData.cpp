/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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

#include "IqrfSensorData.h"
#include "iqrf__IqrfSensorData.hxx"

TRC_INIT_MODULE(iqrf::IqrfSensorData);

namespace iqrf {

	IqrfSensorData::IqrfSensorData() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	IqrfSensorData::~IqrfSensorData() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// API

	bool IqrfSensorData::readInProgress() {
		return m_workerRun && m_exclusiveAccess != nullptr;
	}

	void IqrfSensorData::registerReadingCallback(const std::string &instanceId, std::function<void(bool)> callback) {
		std::lock_guard<std::mutex> lck(m_callbackMutex);
		m_readingCallbacks.insert_or_assign(instanceId, callback);
	}

	void IqrfSensorData::unregisterReadingCallback(const std::string &instanceId) {
		std::lock_guard<std::mutex> lck(m_callbackMutex);
		m_readingCallbacks.erase(instanceId);
	}

	///// Private

	void IqrfSensorData::executeCallbacks(bool inProgress) {
		std::lock_guard<std::mutex> lck(m_callbackMutex);
		for (auto &callback : m_readingCallbacks) {
			if (callback.second) {
				callback.second(inProgress);
			}
		}
	}

	///// Auxiliary methods

	void IqrfSensorData::setErrorTransactionResult(SensorDataResult &result, std::unique_ptr<IDpaTransactionResult2> &transResult, const std::string &errorStr) {
		result.setStatus(transResult->getErrorCode(), errorStr);
		result.addTransactionResult(transResult);
		THROW_EXC(std::logic_error, errorStr);
	}

	uint8_t IqrfSensorData::frcDeviceCountByType(const uint8_t &type) {
		if (type >= 0x01 && type <= 0x7F) {
			return 31;
		}
		if (type >= 0x80 && type <= 0x9F) {
			return 63;
		}
		if (type >= 0xA0 && type <= 0xBF) {
			return 15;
		}
		throw std::domain_error("Unknown or unsupported sensor type.");
	}

	bool IqrfSensorData::extraResultRequired(const uint8_t &command, const uint8_t &deviceCount) {
		switch (command) {
			case FRC_CMD_1BYTE:
				return deviceCount > 55;
			case FRC_CMD_2BYTE:
				return deviceCount > 27;
			case FRC_CMD_4BYTE:
				return deviceCount > 13;
			default:
				throw std::domain_error("Unknown or unsupported FRC command.");
		}
	}

	std::vector<std::set<uint8_t>> IqrfSensorData::splitSet(std::set<uint8_t> &set, size_t size) {
		std::vector<std::set<uint8_t>> res;

		size_t sCount = set.size() / size;
		size_t remainder = set.size() % size;
		auto itr = set.begin();

		for (size_t i = 0; i <= sCount; ++i) {
			std::set<uint8_t> newSet;
			if (i != sCount) {
				newSet.insert(itr, std::next(itr, size));
				std::advance(itr, size);
			} else {
				newSet.insert(itr, std::next(itr, remainder));
			}
			if (newSet.size() > 0) {
				res.push_back(newSet);
			}
		}
		return res;
	}

	///// Message handling

	void IqrfSensorData::setOfflineFrc(SensorDataResult &result) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> transResult;
		try {
			DpaMessage setOfflineFrcRequest;
			DpaMessage::DpaPacket_t setOfflineFrcPacket;
			setOfflineFrcPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			setOfflineFrcPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
			setOfflineFrcPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SET_PARAMS;
			setOfflineFrcPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			setOfflineFrcPacket.DpaRequestPacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams = 0x08;
			setOfflineFrcRequest.DataToBuffer(setOfflineFrcPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerFrcSetParams_RequestResponse));
			// Execute the DPA request
			m_exclusiveAccess->executeDpaTransactionRepeat(setOfflineFrcRequest, transResult, 2);
			TRC_DEBUG("Result from Set FRC params transaction as string: " << PAR(transResult->getErrorString()));
			DpaMessage setOfflineFrcResponse = transResult->getResponse();
			TRC_DEBUG(
				"DPA transaction: "
				<< NAME_PAR(Peripheral type, setOfflineFrcRequest.PeripheralType())
				<< NAME_PAR(Node address, setOfflineFrcRequest.NodeAddress())
				<< NAME_PAR(Command, (int)setOfflineFrcRequest.PeripheralCommand())
			);
			TRC_FUNCTION_LEAVE("");
		} catch (const std::exception &e) {
			setErrorTransactionResult(result, transResult, e.what());
		}
	}

	std::vector<iqrf::sensor::item::Sensor> IqrfSensorData::sendSensorFrc(SensorDataResult &result, const uint8_t &type, const uint8_t &idx, std::set<uint8_t> &nodes) {
		uint8_t command = getSensorFrcCommand(type);
		std::unique_ptr<IDpaTransactionResult2> transResult;
		std::vector<iqrf::sensor::item::Sensor> sensorData;
		try {
			sensor::jsdriver::SensorFrcJs sensorFrc(m_jsRenderService, type, idx, command, nodes);
			sensorFrc.processRequestDrv();
			// frc send selective
			m_dpaService->executeDpaTransactionRepeat(sensorFrc.getFrcRequest(), transResult, 2);
			sensorFrc.setFrcDpaTransactionResult(std::move(transResult));
			// frc extra result
			if (extraResultRequired(command, nodes.size())) {
				m_dpaService->executeDpaTransactionRepeat(sensorFrc.getFrcExtraRequest(), transResult, 2);
				sensorFrc.setFrcExtraDpaTransactionResult(std::move(transResult));
			}
			// handle response
			if (type == 129 || type == 160) {
				auto map = m_dbService->getSensorDeviceHwpids(type);
				sensorFrc.processResponseSensorDrv(map, nodes, true);
			} else {
				sensorFrc.processResponseDrv();
			}
			for (auto &sensor : sensorFrc.getSensors()) {
				sensorData.push_back(*sensor.get());
			}
		} catch (const std::exception &e) {
			setErrorTransactionResult(result, transResult, e.what());
		}
		return sensorData;
	}

	std::vector<uint8_t> IqrfSensorData::frcReadMemory(SensorDataResult &result, std::set<uint8_t> &nodes, const std::vector<uint8_t> &userData) {
		std::unique_ptr<IDpaTransactionResult2> transResult;
		std::vector<std::set<uint8_t>> nodeVectors = splitSet(nodes, 63);
		std::vector<uint8_t> frcData;
		try {
			for (auto vector : nodeVectors) {
				embed::frc::JsDriverSendSelective frcSelective(m_jsRenderService, FRC_MemoryRead, vector, userData);
				frcSelective.processRequestDrv();
				m_dpaService->executeDpaTransactionRepeat(frcSelective.getRequest(), transResult, 2);
				frcSelective.processDpaTransactionResult(std::move(transResult));
				auto data = frcSelective.getFrcData();
				frcData.insert(frcData.end(), data.begin() + 1, data.begin() + 1 + vector.size());

				if (vector.size() > 55) {
					embed::frc::JsDriverExtraResult extraResult(m_jsRenderService);
					extraResult.processRequestDrv();
					m_dpaService->executeDpaTransactionRepeat(extraResult.getRequest(), transResult, 2);
					extraResult.processDpaTransactionResult(std::move(transResult));
					auto extraData = extraResult.getFrcData();
					frcData.insert(frcData.end(), extraData.begin(), extraData.end());
				}
			}
		} catch (const std::exception &e) {
			setErrorTransactionResult(result, transResult, e.what());
		}
		return frcData;
	}

	void IqrfSensorData::getTypeData(SensorDataResult &result, const uint8_t &type, const uint8_t &idx, std::deque<uint8_t> &addresses) {
		const uint8_t devicesPerRequest = frcDeviceCountByType(type);
		const uint8_t v = addresses.size() / devicesPerRequest;
		//const uint8_t r = addresses.size() % devicesPerRequest;
		std::vector<std::set<uint8_t>> vectors;
		for (uint8_t i = 0; i < v; ++i) {
			vectors.push_back(std::set<uint8_t>(addresses.begin(), addresses.begin() + devicesPerRequest));
			addresses.erase(addresses.begin(), addresses.begin() + devicesPerRequest);
		}
		if (addresses.size() > 0) {
			vectors.push_back(std::set<uint8_t>(addresses.begin(), addresses.end()));
		}
		for (auto &vector : vectors) {
			setOfflineFrc(result);
			auto sensorData = sendSensorFrc(result, type, idx, vector);
			for (auto &sensor : sensorData) {
				sensor.setIdx(
					m_dbService->getGlobalSensorIndex(
						sensor.getAddr(),
						sensor.getType(),
						sensor.getIdx()
					)
				);
			}
			result.addSensorData(sensorData);
		}
	}

	void IqrfSensorData::setDeviceHwpidMid(SensorDataResult &result, std::set<uint8_t> &nodes) {
		for (auto &addr : nodes) {
			try {
				uint16_t hwpid = m_dbService->getDeviceHwpid(addr);
				result.setDeviceHwpid(addr, hwpid);
			} catch (const std::exception &e) {
				result.setDeviceHwpid(addr, 0);
			}
			try {
				uint32_t mid = m_dbService->getDeviceMid(addr);
				result.setDeviceMid(addr, mid);
			} catch (const std::exception &e) {
				result.setDeviceMid(addr, 0);
			}
		}
	}

	void IqrfSensorData::getRssi(SensorDataResult &result, std::set<uint8_t> &nodes) {
		auto frcData = frcReadMemory(result, nodes, {182, 5, 2, 0, 0});
		if (nodes.size() == frcData.size()) {
			auto it = nodes.begin();
			for (size_t i = 0; i < nodes.size(); ++i, ++it) {
				result.setDeviceRssi(*it, frcData[i]);
			}
		} else {
			if (frcData.size() == 0) {
				TRC_WARNING("Failed to get device voltage via FRC Memory read.");
			}
		}
	}

	void IqrfSensorData::getRssiBeaming(SensorDataResult &result, std::set<uint8_t> &nodes) {
		std::unique_ptr<IDpaTransactionResult2> transResult;
		std::vector<std::set<uint8_t>> nodeVectors = splitSet(nodes, 63);
		for (auto vector : nodeVectors) {
			try {
				setOfflineFrc(result);
				auto rssiData = sendSensorFrc(result, 133, 0, nodes);
				for (auto item : rssiData) {
					result.setDeviceRssi(item.getAddr(), item.getValue() + 130);
				}
			} catch (const std::exception &e) {
				THROW_EXC_TRC_WAR(std::logic_error, e.what());
			}
		}
	}

	void IqrfSensorData::getDataByFrc(SensorDataResult &result) {
		std::set<uint8_t> allNodes = m_dbService->getDeviceAddrs();
		if (allNodes.count(0) > 0) {
			allNodes.erase(0);
		}
		// fill HWPID and MID
		setDeviceHwpidMid(result, allNodes);
		// TODO: rssi from beaming devices - split nodes into ones that can get data using offline frc (beaming and aggregating repeaters)
		// TODO: this requires metadata
		getRssiBeaming(result, allNodes);
		auto noRssi = result.getNodesWithoutRssi();
		if (noRssi.size() > 0) {
			getRssi(result, noRssi);
		}
		SensorSelectMap map = m_dbService->constructSensorSelectMap();
		std::set<uint8_t> sensorNodes;
		for (const auto& [type, addrIdx] : map) {
			TRC_DEBUG("type: " << std::to_string(type));
			if (type >= 0xC0) {
				continue;
			}
			for (uint8_t desiredIdx = 0; desiredIdx < 32; ++desiredIdx) {
				std::deque<uint8_t> addrs;
				for (const auto& [addr, idx] : addrIdx) {
					sensorNodes.insert(addr);
					if (desiredIdx == idx) {
						addrs.emplace_back(addr);
					}
				}
				if (addrs.size() > 0) {
					getTypeData(result, type, desiredIdx, addrs);
				}
			}
		}
	}

	void IqrfSensorData::worker() {
		TRC_FUNCTION_ENTER("");

		while (m_workerRun) {
			try {
				m_exclusiveAccess = m_dpaService->getExclusiveAccess();
			} catch (const std::exception &e) {
				CATCH_EXC_TRC_WAR(std::exception, e, "Failed to acquire exclusive access: " << e.what());
				std::unique_lock<std::mutex> lock(m_mtx);
				TRC_DEBUG("Exlusive access acquisition failed, retrying in " << m_retryPeriod << " minutes.");
				m_cv.wait_for(lock, std::chrono::minutes(m_retryPeriod));
				continue;
			}

			auto nextReadingTime = std::chrono::steady_clock::now() + std::chrono::minutes(m_period);

			try {
				executeCallbacks(true);
				SensorDataResult result;
				if (m_asyncReports) {
					Document doc;
					result.setMessageType(m_mTypeReportAsync);
					result.setMessageId("async");
					result.createStartMessage(doc);
					m_splitterService->sendMessage(m_messagingList, std::move(doc));
				}
				getDataByFrc(result);
				m_dbService->updateSensorValues(result.getSensorData());
				m_exclusiveAccess.reset();
				if (m_asyncReports) {
					Document doc;
					result.setMessageType(m_mTypeReportAsync);
					result.setMessageId("async");
					result.createResultMessage(doc);
					m_splitterService->sendMessage(m_messagingList, std::move(doc));
				}
			} catch (const std::exception &e) {
				CATCH_EXC_TRC_WAR(std::exception, e, e.what());
				m_exclusiveAccess.reset();
			}

			executeCallbacks(false);

			auto nowTime = std::chrono::steady_clock::now();
			auto waitTimeSec = std::chrono::duration_cast<std::chrono::seconds>(nextReadingTime - nowTime).count();

			if (waitTimeSec > 0) {
				std::unique_lock<std::mutex> lock(m_mtx);
				TRC_DEBUG("Sensor data worker thread sleeping for: " + std::to_string(waitTimeSec) + " seconds.");
				m_cv.wait_for(lock, std::chrono::seconds(waitTimeSec));
			}
		}

		TRC_FUNCTION_LEAVE("");
	}

	void IqrfSensorData::workerStatus(rapidjson::Document &request, const MessagingInstance &messaging) {
		TRC_FUNCTION_ENTER("");

		bool running = m_workerRun;
		bool reading = m_workerRun && m_exclusiveAccess != nullptr;

		Document rsp;
		Pointer("/mType").Set(rsp, m_mTypeStatus);
		Pointer("/data/msgId").Set(rsp, Pointer("/data/msgId").Get(request)->GetString());
		Pointer("/data/rsp/running").Set(rsp, running);
		Pointer("/data/rsp/reading").Set(rsp, reading);
		Pointer("/data/status").Set(rsp, 0);
		Pointer("/data/statusStr").Set(rsp, "ok");
		m_splitterService->sendMessage(messaging, std::move(rsp));
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfSensorData::notifyWorker(rapidjson::Document &request, const MessagingInstance &messaging) {
		TRC_FUNCTION_ENTER("");

		bool invoked = false;
		bool running = false;

		if (m_workerRun) {
			running = true;
			if (!m_exclusiveAccess) {
				m_cv.notify_all();
				invoked = true;
			}
		}

		Document rsp;

		Pointer("/mType").Set(rsp, m_mTypeInvoke);
		Pointer("/data/msgId").Set(rsp, Pointer("/data/msgId").Get(request)->GetString());
		if (invoked && running) {
			Pointer("/data/status").Set(rsp, 0);
			Pointer("/data/statusStr").Set(rsp, "ok");
		} else {
			if (!running) {
				Pointer("/data/status").Set(rsp, ErrorCodes::notRunning);
				Pointer("/data/statusStr").Set(rsp, "Sensor data read worker not running.");
			} else {
				Pointer("/data/status").Set(rsp, ErrorCodes::readingInProgress);
				Pointer("/data/statusStr").Set(rsp, "Sensor data read already in progress.");
			}
		}
		m_splitterService->sendMessage(messaging, std::move(rsp));
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfSensorData::startWorker(rapidjson::Document &request, const MessagingInstance &messaging) {
		TRC_FUNCTION_ENTER("");

		if (!m_workerRun) {
			if (m_workerThread.joinable()) {
				m_workerThread.join();
			}
			m_workerRun = true;
			m_workerThread = std::thread([&]() {
				worker();
			});
		}

		Document rsp;

		Pointer("/mType").Set(rsp, m_mTypeStart);
		Pointer("/data/msgId").Set(rsp, Pointer("/data/msgId").Get(request)->GetString());
		Pointer("/data/status").Set(rsp, 0);
		Pointer("/data/statusStr").Set(rsp, "ok");
		m_splitterService->sendMessage(messaging, std::move(rsp));
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfSensorData::stopWorker(rapidjson::Document &request, const MessagingInstance &messaging) {
		TRC_FUNCTION_ENTER("");

		if (m_workerRun) {
			m_workerRun = false;
			m_cv.notify_all();
			if (m_workerThread.joinable()) {
				m_workerThread.join();
			}
		}

		Document rsp;

		Pointer("/mType").Set(rsp, m_mTypeStop);
		Pointer("/data/msgId").Set(rsp, Pointer("/data/msgId").Get(request)->GetString());
		Pointer("/data/status").Set(rsp, 0);
		Pointer("/data/statusStr").Set(rsp, "ok");
		m_splitterService->sendMessage(messaging, std::move(rsp));
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfSensorData::getConfig(rapidjson::Document &request, const MessagingInstance &messaging) {
		TRC_FUNCTION_ENTER("");

		Document rsp;
		Document::AllocatorType &allocator = rsp.GetAllocator();

		Pointer("/mType").Set(rsp, m_mTypeGetConfig);
		Pointer("/data/msgId").Set(rsp, Pointer("/data/msgId").Get(request)->GetString());
		Pointer("/data/rsp/autoRun").Set(rsp, m_autoRun);
		Pointer("/data/rsp/period").Set(rsp, m_period);
		Pointer("/data/rsp/retryPeriod").Set(rsp, m_retryPeriod);
		Pointer("/data/rsp/asyncReports").Set(rsp, m_asyncReports);
		Value arr(kArrayType);
		for (auto &item : m_messagingList) {
			Value messagingObject;
			Pointer("/type").Set(messagingObject, MessagingConversion::messagingTypeToString(item.type).c_str(), allocator);
			Pointer("/instance").Set(messagingObject, item.instance, allocator);
			arr.PushBack(messagingObject, allocator);
		}
		Pointer("/data/rsp/messagingList").Set(rsp, arr, allocator);
		Pointer("/data/status").Set(rsp, 0);
		Pointer("/data/statusStr").Set(rsp, "ok");
		m_splitterService->sendMessage(messaging, std::move(rsp));
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfSensorData::setConfig(rapidjson::Document &request, const MessagingInstance &messaging)  {
		TRC_FUNCTION_ENTER("");

		Document rsp;
		Pointer("/mType").Set(rsp, m_mTypeSetConfig);
		Pointer("/data/msgId").Set(rsp, Pointer("/data/msgId").Get(request)->GetString());

		auto oldAutoRun = m_autoRun;
		auto oldPeriod = m_period;
		auto oldRetryPeriod = m_retryPeriod;
		auto oldAsyncReports = m_asyncReports;
		auto oldMessagingList = m_messagingList;

		shape::IConfiguration *cfg = m_configService->getConfiguration(m_componentName, m_instanceName);
		try {
			if (!cfg) {
				throw std::logic_error("Failed to load configuration");
			}

			Document &cfgDoc = cfg->getProperties()->getAsJson();
			Document::AllocatorType &allocator = cfgDoc.GetAllocator();

			const Value *val = Pointer("/data/req/autoRun").Get(request);
			if (val && val->IsBool()) {
				m_autoRun = val->GetBool();
				Pointer("/autoRun").Set(cfgDoc, m_autoRun);
			}

			val = Pointer("/data/req/period").Get(request);
			if (val && val->IsUint()) {
				m_period = val->GetUint();
				Pointer("/period").Set(cfgDoc, m_period);
			}

			val = Pointer("/data/req/retryPeriod").Get(request);
			if (val && val->IsUint()) {
				m_retryPeriod = val->GetUint();
				Pointer("/retryPeriod").Set(cfgDoc, m_retryPeriod);
			}

			val = Pointer("/data/req/asyncReports").Get(request);
			if (val && val->IsBool()) {
				m_asyncReports = val->GetBool();
				Pointer("/asyncReports").Set(cfgDoc, m_asyncReports);
			}

			val = Pointer("/data/req/messagingList").Get(request);
			if (val && val->IsArray()) {
				std::list<MessagingInstance> list = {};
				auto arr = val->GetArray();
				for (auto itr = arr.Begin(); itr != arr.End(); ++itr) {
					list.push_back(
						MessagingInstance(
							Pointer("/type").Get(*itr)->GetString(),
							Pointer("/instance").Get(*itr)->GetString()
						)
					);
				}
				m_messagingList = list;
				Pointer("/messagingList").Set(cfgDoc, *val, allocator);
			}

			cfg->update(true);

			Pointer("/data/status").Set(rsp, 0);
			Pointer("/data/statusStr").Set(rsp, "ok");
		} catch (const std::exception &e) {
			CATCH_EXC_TRC_WAR(std::exception, e, e.what());
			m_autoRun = oldAutoRun;
			m_period = oldPeriod;
			m_retryPeriod = oldRetryPeriod;
			m_asyncReports = oldAsyncReports;
			m_messagingList = oldMessagingList;
			Pointer("/data/status").Set(rsp, 1005);
			Pointer("/data/statusStr").Set(rsp, "Failed to load and update component instance configuration.");
		}

		m_splitterService->sendMessage(messaging, std::move(rsp));
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfSensorData::handleMsg(const MessagingInstance &messaging, const IMessagingSplitterService::MsgType &msgType, Document doc) {
		TRC_FUNCTION_ENTER(
			PAR(messaging.to_string()) <<
			NAME_PAR(mType, msgType.m_type) <<
			NAME_PAR(major, msgType.m_major) <<
			NAME_PAR(minor, msgType.m_minor) <<
			NAME_PAR(patch, msgType.m_micro)
		);

		if (msgType.m_type == m_mTypeStatus) {
			workerStatus(doc, messaging);
		} else if (msgType.m_type == m_mTypeInvoke) {
			notifyWorker(doc, messaging);
		} else if (msgType.m_type == m_mTypeStart) {
			startWorker(doc, messaging);
		} else if (msgType.m_type == m_mTypeStop) {
			stopWorker(doc, messaging);
		} else if (msgType.m_type == m_mTypeGetConfig) {
			getConfig(doc, messaging);
		} else {
			setConfig(doc, messaging);
		}
		TRC_FUNCTION_LEAVE("");
	}

	///// Component management

	void IqrfSensorData::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "IqrfSensorData instance activate" << std::endl
			<< "******************************"
		);

		modify(props);
		if (m_autoRun) {
			m_workerRun = true;
			m_workerThread = std::thread([&]() {
				worker();
			});
		}

		m_splitterService->registerFilteredMsgHandler(
			std::vector<std::string>{
				m_mTypeGetConfig,
				m_mTypeSetConfig,
				m_mTypeStatus,
				m_mTypeInvoke,
				m_mTypeStart,
				m_mTypeStop
			},
			[&](const MessagingInstance &messaging, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc) {
				handleMsg(messaging, msgType, std::move(doc));
			}
		);
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfSensorData::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");

		const Document &doc = props->getAsJson();

		m_componentName = Pointer("/component").Get(doc)->GetString();
		m_instanceName = Pointer("/instance").Get(doc)->GetString();

		m_autoRun = Pointer("/autoRun").Get(doc)->GetBool();
		m_period = Pointer("/period").Get(doc)->GetUint();
		m_asyncReports = Pointer("/asyncReports").Get(doc)->GetBool();
		m_retryPeriod = Pointer("/retryPeriod").Get(doc)->GetUint();

		m_messagingList.clear();
		const Value *val = Pointer("/messagingList").Get(doc);
		if (val && val->IsArray()) {
			const auto arr = val->GetArray();
			for (auto itr = arr.Begin(); itr != arr.End(); ++itr) {
				m_messagingList.push_back(
					MessagingInstance(
						Pointer("/type").Get(*itr)->GetString(),
						Pointer("/instance").Get(*itr)->GetString()
					)
				);
			}
		}

		TRC_FUNCTION_LEAVE("");
	}

	void IqrfSensorData::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "IqrfSensorData instance deactivate" << std::endl
			<< "******************************"
		);

		m_workerRun = false;
		m_cv.notify_all();
		if (m_workerThread.joinable()) {
			m_workerThread.join();
		}

		m_splitterService->unregisterFilteredMsgHandler(std::vector<std::string>{
			m_mTypeGetConfig,
			m_mTypeSetConfig,
			m_mTypeStatus,
			m_mTypeInvoke,
			m_mTypeStart,
			m_mTypeStop
		});

		TRC_FUNCTION_LEAVE("");
	}

	///// Interface management

	void IqrfSensorData::attachInterface(shape::IConfigurationService *iface) {
		m_configService = iface;
	}

	void IqrfSensorData::detachInterface(shape::IConfigurationService *iface) {
		if (m_configService == iface) {
			m_configService = nullptr;
		}
	}

	void IqrfSensorData::attachInterface(IIqrfDb *iface) {
		m_dbService = iface;
	}

	void IqrfSensorData::detachInterface(IIqrfDb *iface) {
		if (m_dbService == iface) {
			m_dbService = nullptr;
		}
	}

	void IqrfSensorData::attachInterface(IIqrfDpaService *iface) {
		m_dpaService = iface;
	}

	void IqrfSensorData::detachInterface(IIqrfDpaService *iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void IqrfSensorData::attachInterface(IJsRenderService *iface) {
		m_jsRenderService = iface;
	}

	void IqrfSensorData::detachInterface(IJsRenderService *iface) {
		if (m_jsRenderService == iface) {
			m_jsRenderService = nullptr;
		}
	}

	void IqrfSensorData::attachInterface(IMessagingSplitterService *iface) {
		m_splitterService = iface;
	}

	void IqrfSensorData::detachInterface(IMessagingSplitterService *iface) {
		if (m_splitterService == iface) {
			m_splitterService = nullptr;
		}
	}

	void IqrfSensorData::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void IqrfSensorData::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}
}
