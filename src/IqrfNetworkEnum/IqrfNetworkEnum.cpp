/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

#include "IqrfNetworkEnum.h"

#include "iqrf__IqrfNetworkEnum.hxx"

TRC_INIT_MODULE(iqrf::IqrfNetworkEnum);

namespace iqrf {

  IqrfNetworkEnum::IqrfNetworkEnum() {
    TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
  }

  IqrfNetworkEnum::~IqrfNetworkEnum() {
    TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
  }

  ///// Component lifecycle

  void IqrfNetworkEnum::activate(const shape::Properties *props) {
    TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************" << std::endl <<
			"IqrfNetworkEnum instance activate" << std::endl <<
			"******************************"
		);
		modify(props);
		std::vector<std::string> messages = {m_enumerateMsg, m_enumerateAsyncMsg};
		m_splitterService->registerFilteredMsgHandler(messages, [&](const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document request) {
			handleMsg(messagingId, msgType, std::move(request));
		});
    m_cacheService->registerCacheReloadedHandler(m_instance, [&]() {
			reloadDrivers();
		});
		m_dpaService->registerAnyMessageHandler(m_instance, [&](const DpaMessage &msg) {
			analyzeDpaMessage(msg);
		});
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
  }

  void IqrfNetworkEnum::modify(const shape::Properties *props) {
    TRC_FUNCTION_ENTER("");
    using namespace rapidjson;
    // read configuration parameters
		const Document &doc = props->getAsJson();
		m_instance = Pointer("/instance").Get(doc)->GetString();
		m_autoEnumerateBeforeInvoked = Pointer("/autoEnumerateBeforeInvoked").Get(doc)->GetBool();
		m_enumerateOnLaunch = Pointer("/enumerateOnLaunch").Get(doc)->GetBool();
		TRC_FUNCTION_LEAVE("");
  }

  void IqrfNetworkEnum::deactivate() {
    TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************" << std::endl <<
			"IqrfNetworkEnum instance deactivate" << std::endl <<
			"******************************"
		);
		m_enumThreadRun = false;
		stopEnumerationThread();
		std::vector<std::string> messages = {m_enumerateMsg, m_enumerateAsyncMsg};
		m_splitterService->unregisterFilteredMsgHandler(messages);
		m_cacheService->unregisterCacheReloadedHandler(m_instance);
		m_dpaService->unregisterAnyMessageHandler(m_instance);
		clearAuxBuffers();
		TRC_FUNCTION_LEAVE("");
  }

  ///// API

  void IqrfNetworkEnum::enumerate(IIqrfNetworkEnum::EnumParams &parameters) {
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

  bool IqrfNetworkEnum::isRunning() {
		return m_exclusiveAccess != nullptr && m_enumRun;
	}

  void IqrfNetworkEnum::registerEnumerationHandler(const std::string &clientId, EnumerationHandler handler) {
		std::lock_guard<std::mutex> lock(m_enumMutex);
		m_enumHandlers.insert(std::make_pair(clientId, handler));
	}

	void IqrfNetworkEnum::unregisterEnumerationHandler(const std::string &clientId) {
		std::lock_guard<std::mutex> lock(m_enumMutex);
		m_enumHandlers.erase(clientId);
	}

  void IqrfNetworkEnum::reloadDrivers() {
      TRC_FUNCTION_ENTER("");

      if (m_renderService != nullptr) {
        m_renderService->clearContexts();
      }
      loadCoordinatorDrivers();
      loadProductDrivers();

      TRC_FUNCTION_LEAVE("");
    }

	void IqrfNetworkEnum::reloadCoordinatorDrivers() {
		TRC_FUNCTION_ENTER("");
		loadCoordinatorDrivers();
		TRC_FUNCTION_LEAVE("");
	}

  ///// Private

  void IqrfNetworkEnum::startEnumerationThread(IIqrfNetworkEnum::EnumParams &parameters) {
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

	void IqrfNetworkEnum::stopEnumerationThread() {
		TRC_FUNCTION_ENTER("");
		m_enumRun = false;
		m_enumCv.notify_all();
		if (m_enumThread.joinable()) {
			m_enumThread.join();
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfNetworkEnum::analyzeDpaMessage(const DpaMessage &message) {
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

	void IqrfNetworkEnum::runEnumeration(IIqrfNetworkEnum::EnumParams &parameters) {
		TRC_FUNCTION_ENTER("");

		m_params = parameters;

		while (m_enumThreadRun) {
			if (m_enumRun) {
				if (!m_dpaService->hasExclusiveAccess()) {
					waitForExclusiveAccess();
					TRC_INFORMATION("Running enumeration with: " << PAR(m_params.reenumerate) << PAR(m_params.standards));
					sendEnumerationProgressMessage(EnumerationProgress(EnumerationProgress::Steps::Start));
					checkNetwork(m_params.reenumerate);
					sendEnumerationProgressMessage(EnumerationProgress(EnumerationProgress::Steps::NetworkDone));
					resetExclusiveAccess();

					if (!m_enumThreadRun) {
						break;
					}

					waitForExclusiveAccess();
					sendEnumerationProgressMessage(EnumerationProgress(EnumerationProgress::Steps::Devices));
					enumerateDevices();
					sendEnumerationProgressMessage(EnumerationProgress(EnumerationProgress::Steps::DevicesDone));
					resetExclusiveAccess();

					if (!m_enumThreadRun) {
						break;
					}

					waitForExclusiveAccess();
					sendEnumerationProgressMessage(EnumerationProgress(EnumerationProgress::Steps::Products));
					productPackageEnumeration();
					updateDatabaseProducts();
					loadProductDrivers();
					sendEnumerationProgressMessage(EnumerationProgress(EnumerationProgress::Steps::ProductsDone));
					resetExclusiveAccess();

					if (!m_enumThreadRun) {
						break;
					}

					if (m_params.standards || m_params.reenumerate) {
						waitForExclusiveAccess();
						sendEnumerationProgressMessage(EnumerationProgress(EnumerationProgress::Steps::Standards));
						standardEnumeration();
						sendEnumerationProgressMessage(EnumerationProgress(EnumerationProgress::Steps::StandardsDone));
						resetExclusiveAccess();
					}
					m_enumRepeat = false;
					sendEnumerationProgressMessage(EnumerationProgress(EnumerationProgress::Steps::Finish));
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

	void IqrfNetworkEnum::checkNetwork(bool reenumerate) {
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

		auto dbDevices = m_dbService->getAllDevices();

		for (auto it = dbDevices.begin(); it != dbDevices.end(); ++it) {
			iqrf::db::Device device = *it;
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

	void IqrfNetworkEnum::enumerateDevices() {
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

	void IqrfNetworkEnum::getBondedNodes() {
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

	void IqrfNetworkEnum::getDiscoveredNodes() {
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

	void IqrfNetworkEnum::getMids() {
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

	void IqrfNetworkEnum::getRoutingInformation() {
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

	void IqrfNetworkEnum::coordinatorEnumeration() {
		uint16_t osBuild = m_coordinatorParams.osBuildWord;
		std::string osVersion = IqrfCommon::osVersionString(m_coordinatorParams.osVersionByte, m_coordinatorParams.trMcuType);
		uint16_t dpaVersion = m_coordinatorParams.dpaVerWord;
		uint16_t hwpid = m_coordinatorParams.hwpid;
		uint16_t hwpidVersion = m_coordinatorParams.hwpidVersion;
		UniqueProduct uniqueProduct = std::make_tuple(hwpid, hwpidVersion, osBuild, dpaVersion);
		db::Product product(hwpid, hwpidVersion, osBuild, osVersion, dpaVersion);
		m_productMap.insert(std::make_pair(uniqueProduct, product));
		m_deviceProductMap.insert(std::make_pair(0, std::make_shared<db::Product>(m_productMap[uniqueProduct])));
	}

	void IqrfNetworkEnum::pollEnumeration() {
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
				std::string osVersion = IqrfCommon::osVersionString(osRead.OsVersion, osRead.McuType);
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
					db::Product product(hwpid, hwpidVersion, osBuild, osVersion, dpaVersion);
					m_productMap.insert(std::make_pair(uniqueProduct, product));
				}
				m_deviceProductMap.insert(std::make_pair(*it, std::make_shared<db::Product>(m_productMap[uniqueProduct])));
				++it;
			} catch (const std::exception &e) {
				TRC_WARNING("Failed to enumerate node at address " << static_cast<unsigned>(*it) << ": " << e.what());
				m_toEnumerate.erase(*it++);
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfNetworkEnum::frcEnumeration() {
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
				db::Product product(hwpid, hwpidVersion, osBuild, osVersion, dpa);
				m_productMap.insert(std::make_pair(uniqueProduct, product));
			}
			m_deviceProductMap.insert(std::make_pair(addr, std::make_shared<db::Product>(m_productMap[uniqueProduct])));
		}
		TRC_FUNCTION_LEAVE("");
	}

	///// Requests

	void IqrfNetworkEnum::eeepromRead(uint8_t* data, const uint16_t &address, const uint8_t &len) {
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

	void IqrfNetworkEnum::frcHwpid(std::map<uint8_t, HwpidTuple> *hwpidMap, uint8_t &frcCount, uint8_t &nodes, uint8_t &remainingNodes) {
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

	void IqrfNetworkEnum::frcDpa(std::map<uint8_t, uint16_t> *dpaMap, uint8_t &frcCount, uint8_t &nodes, uint8_t &remainingNodes) {
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

	void IqrfNetworkEnum::frcOs(std::map<uint8_t, OsTuple> *osMap, uint8_t &frcCount, uint8_t &nodes, uint8_t &remainingNodes) {
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
			OsTuple tuple = std::make_tuple(osBuild, IqrfCommon::osVersionString(frcData[i], frcData[i + 1]));
			osMap->insert(std::make_pair(addr, tuple));
			i+= 4;
		}
		TRC_FUNCTION_LEAVE("");
	}

	const std::set<uint8_t> IqrfNetworkEnum::frcPing() {
		TRC_FUNCTION_ENTER("");
		std::set<uint8_t> onlineNodes;
		std::unique_ptr<IDpaTransactionResult2> result;
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

	void IqrfNetworkEnum::frcSendSelectiveMemoryRead(uint8_t* data, const uint16_t &address, const uint8_t &pnum, const uint8_t &pcmd, const uint8_t &numNodes, const uint8_t &processedNodes) {
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
			std::vector<uint8_t> nodes = selectNodes(m_toEnumerate, processedNodes, numNodes);
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

	void IqrfNetworkEnum::frcExtraResult(uint8_t *data) {
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

	void IqrfNetworkEnum::productPackageEnumeration() {
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
				auto dbProduct = m_dbService->getProduct(hwpid, hwpidVersion, osBuild, dpaVersion);
				if (dbProduct != nullptr) {
					// found in db, enumerate from existing record
					product->setHandlerUrl(dbProduct->getHandlerUrl());
					product->setHandlerHash(dbProduct->getHandlerHash());
					product->setNotes(dbProduct->getNotes());
					product->setCustomDriver(dbProduct->getCustomDriver());
					product->setPackageId(dbProduct->getPackageId());
					auto driverIds =  m_dbService->getDriverIdsByProduct(dbProduct->getId());
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
				auto dbDriver = m_dbService->getDriver(per, version);
				if (dbDriver == nullptr) {
					db::Driver driver(item.getName(), per, version, item.getVersionFlags(), *item.getNotes(), *item.getDriver());
					uint32_t driverId = m_dbService->insertDriver(driver);
					product->drivers.insert(driverId);
				} else {
					product->drivers.insert(dbDriver->getId());
				}
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfNetworkEnum::updateDatabaseProducts() {
		TRC_FUNCTION_ENTER("");
		using namespace sqlite_orm;
		m_dbService->beginTransaction();
		for (auto &deleteId : m_toDelete) {
			m_dbService->removeDevice(deleteId);
		}
		uint32_t productId = -1;
		for (auto &addr : m_toEnumerate) {
			ProductPtr &product = m_deviceProductMap[addr];
			try {
				// create a new product or update existing
				auto dbProduct = m_dbService->getProduct(
					product->getHwpid(),
					product->getHwpidVersion(),
					product->getOsBuild(),
					product->getDpaVersion()
				);
				if (dbProduct == nullptr) {
					productId = m_dbService->insertProduct(*product.get());
					for (auto &driver : product->drivers) {
						db::ProductDriver productDriver(productId, driver);
						m_dbService->insertProductDriver(productDriver);
					}
				} else {
					productId = dbProduct->getId();
				}

				// create a new device or update existing
				bool discovered = m_discovered.find(addr) != m_discovered.end();
				uint32_t mid = m_mids[addr];
				uint8_t vrn = discovered ? m_vrns[addr] : 0;
				uint8_t zone = discovered ? m_zones[addr] : 0;
				std::shared_ptr<uint8_t> parent = discovered ? std::make_shared<uint8_t>(m_parents[addr]) : nullptr;
				auto dbDevice = m_dbService->getDevice(addr);
				if (dbDevice == nullptr) {
					// create new
					db::Device device(addr, discovered, mid, vrn, zone, parent);
					device.setProductId(productId);
					m_dbService->insertDevice(device);
				} else {
					// update existing
					db::Device device = *dbDevice.get();
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
					m_dbService->updateDevice(device);
				}
			} catch (const std::system_error &e) {
				CATCH_EXC_TRC_WAR(const std::system_error, e, "Failed to insert entity to database - [" << e.code() << "]: " << e.what());
				m_dbService->cancelTransaction();
				return;
			}
		}
		m_dbService->finishTransaction();
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfNetworkEnum::standardEnumeration() {
		TRC_FUNCTION_ENTER("");
		using namespace sqlite_orm;
		// select devices to enumerate
		std::map<uint32_t, uint8_t> devices;
		for (auto &device : m_dbService->getAllDevices()) {
			if (!device.isEnumerated() || m_params.reenumerate) {
				devices.insert(std::make_pair(device.getId(), device.getAddress()));
			}
		}

		for (auto &device : devices) {
			uint32_t deviceId = device.first;
			uint8_t address = device.second;

			// begin transaction
			m_dbService->beginTransaction();
			try {
				if (m_dbService->deviceImplementsPeripheral(deviceId, PERIPHERAL_BINOUT)) {
					binoutEnumeration(deviceId, address);
				} else {
					m_dbService->removeBinaryOutput(deviceId);
				}
				if (m_dbService->deviceImplementsPeripheral(deviceId, PERIPHERAL_DALI)) {
					daliEnumeration(deviceId);
				} else {
					m_dbService->removeDali(deviceId);
				}
				if (m_dbService->deviceImplementsPeripheral(deviceId, PERIPHERAL_LIGHT)) {
					lightEnumeration(deviceId, address);
				} else {
					m_dbService->removeLight(deviceId);
				}
				if (m_dbService->deviceImplementsPeripheral(deviceId, PERIPHERAL_SENSOR)) {
					sensorEnumeration(address);
				} else {
					m_dbService->removeDeviceSensors(address);
				}
				// set as enumerated
				auto dbDevice = m_dbService->getDevice(deviceId);
				dbDevice->setEnumerated(true);
				m_dbService->updateDevice(*dbDevice.get());
				m_dbService->finishTransaction();
			} catch (const std::exception &e) {
				m_dbService->cancelTransaction();
				CATCH_EXC_TRC_WAR(std::exception, e, e.what());
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfNetworkEnum::binoutEnumeration(const uint32_t &deviceId, const uint8_t &address) {
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

		auto dbBo = m_dbService->getBinaryOutputByDevice(deviceId);
		if (dbBo == nullptr) {
			db::BinaryOutput binaryOutput(deviceId, count);
			m_dbService->insertBinaryOutput(binaryOutput);
		} else {
			dbBo->setCount(count);
			m_dbService->updateBinaryOutput(*dbBo.get());
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfNetworkEnum::daliEnumeration(const uint32_t &deviceId) {
		TRC_FUNCTION_ENTER("");
		using namespace sqlite_orm;

		auto dbDali = m_dbService->getDaliByDevice(deviceId);
		if (dbDali == nullptr) {
			db::Dali dali(deviceId);
			m_dbService->insertDali(dali);
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfNetworkEnum::lightEnumeration(const uint32_t &deviceId, const uint8_t &address) {
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

		auto dbLight = m_dbService->getLightByDevice(deviceId);
		if (dbLight == nullptr) {
			db::Light light(deviceId, count);
			m_dbService->insertLight(light);
		} else {
			dbLight->setCount(count);
			m_dbService->updateLight(*dbLight.get());
		}
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfNetworkEnum::sensorEnumeration(const uint8_t &address) {
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
			auto dbSensor = m_dbService->getSensor(type, name);
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
				db::Sensor sensor(type, name, shortname, unit, decimals, frc2Bit, frc1Byte, frc2Byte, frc4Byte);
				// Store new sensor and get ID
				sensorId = m_dbService->insertSensor(sensor);
			}

			// store device and sensor
			const uint8_t index = item->getIdx();
			auto dbDeviceSensor = m_dbService->getDeviceSensor(address, sensorId, index);
			if (dbDeviceSensor == nullptr) {
				db::DeviceSensor deviceSensor(address, type, index, cnt[type], sensorId, nullptr);
				m_dbService->insertDeviceSensor(deviceSensor);
				cnt[type] += 1;
			}
		}
		TRC_FUNCTION_LEAVE("");
	}

  void IqrfNetworkEnum::clearAuxBuffers() {
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

	void IqrfNetworkEnum::waitForExclusiveAccess() {
		std::unique_lock<std::mutex> lock(m_enumMutex);
		while (m_exclusiveAccessCv.wait_for(lock, std::chrono::seconds(1), [&] {
			return m_dpaService->hasExclusiveAccess();
		}));
		m_exclusiveAccess = m_dpaService->getExclusiveAccess();
		TRC_DEBUG("Exclusive access acquired.");
	}

	void IqrfNetworkEnum::resetExclusiveAccess() {
		std::unique_lock<std::mutex> lock(m_enumMutex);
		if (m_exclusiveAccess != nullptr) {
			m_exclusiveAccess.reset();
			TRC_DEBUG("Exclusive access released.");
		}
	}

  void IqrfNetworkEnum::loadCoordinatorDrivers() {
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

					drivers = m_cacheService->getDrivers(IqrfCommon::osBuildString(osBuild), IqrfCommon::dpaVersionHexaString(dpaVersion));
					if (drivers.size() > 0) {
						oss << std::endl
							<< "Loaded drivers for OS " << IqrfCommon::osBuildString(osBuild)
							<< ", DPA " << IqrfCommon::dpaVersionHexaString(dpaVersion);
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

	void IqrfNetworkEnum::loadProductDrivers() {
		TRC_FUNCTION_ENTER("");
		using namespace sqlite_orm;
		std::string wrapper = loadWrapper();

		try {
			std::map<uint32_t, std::set<uint32_t>> productsDrivers = m_dbService->getProductsDriversIdMap();
			std::set<uint8_t> reloadDevices;

			uint32_t coordinatorProductId = m_dbService->getCoordinatorProductId();

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
					std::vector<db::Driver> drivers;
					if (productId == coordinatorProductId) { // ensure standard FRC backwards compatibility
						drivers = m_dbService->getLatestDrivers();
					} else {
						drivers = m_dbService->getDriversByProduct(productId);
					}
					std::string customDriver = m_dbService->getProductCustomDriver(productId);

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

					std::vector<uint8_t> addresses = m_dbService->getProductDeviceAddresses(productId);

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

	void IqrfNetworkEnum::handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc) {
		using namespace rapidjson;
		TRC_FUNCTION_ENTER(
			PAR(messagingId) <<
			NAME_PAR(mType, msgType.m_type) <<
			NAME_PAR(major, msgType.m_major) <<
			NAME_PAR(minor, msgType.m_minor) <<
			NAME_PAR(patch, msgType.m_micro)
		);

		EnumParams params;
		const Value *v = Pointer("/data/req/reenumerate").Get(doc);
		if (v && v->IsBool()) {
			params.reenumerate = v->GetBool();
		}
		params.standards = Pointer("/data/req/standards").Get(doc)->GetBool();
		enumerate(params);

		Document rsp;
		Document::AllocatorType &allocator = rsp.GetAllocator();
		Pointer("/mType").Set(rsp, m_enumerateMsg);
		Pointer("/data/msgId").Set(rsp, Pointer("/data/msgId").Get(doc)->GetString());
		Pointer("/data/rsp").Set(rsp, Pointer("/data/req").Get(doc)->GetObject(), allocator);
		Pointer("/data/status").Set(rsp, 0);
		Pointer("/data/statusStr").Set(rsp, "ok");
		m_splitterService->sendMessage(messagingId, std::move(rsp));
	}

	///// Messages

	void IqrfNetworkEnum::sendEnumerationProgressMessage(EnumerationProgress progress) {
		using namespace rapidjson;

		Document rsp;
		Pointer("/mType").Set(rsp, m_enumerateAsyncMsg);
		Pointer("/data/msgId").Set(rsp, "iqrf_network_enumeration_async");
		Pointer("/data/rsp/step").Set(rsp, progress.getStep());
		Pointer("/data/rsp/stepStr").Set(rsp, progress.getStepMessage());
		Pointer("/data/status").Set(rsp, 0);
		Pointer("/data/statusStr").Set(rsp, "ok");
		m_splitterService->sendMessage("", std::move(rsp));
	}

	///// Auxiliary functions /////

	std::string IqrfNetworkEnum::loadWrapper() {
		std::string path = m_launchService->getDataDir() + "/javaScript/DaemonWrapper.js";
		std::ifstream file(path);
		if (!file.is_open()) {
			THROW_EXC_TRC_WAR(std::logic_error, "Failed to open file wrapper file: " << path);
		}
		std::stringstream ss;
		ss << file.rdbuf();
		return ss.str();
	}

	std::vector<uint8_t> IqrfNetworkEnum::selectNodes(const std::set<uint8_t> &nodes, const uint8_t &idx, const uint8_t &count) {
		std::vector<uint8_t> selectedNodes(30, 0);
		std::set<uint8_t>::iterator it = nodes.begin();
		std::advance(it, idx);
		for (std::set<uint8_t>::iterator end = std::next(it, count); it != end; it++) {
			selectedNodes[*it / 8] |= (1 << (*it % 8));
		}
		return selectedNodes;
	}

  ///// Service interfaces

	void IqrfNetworkEnum::attachInterface(IIqrfDb *iface) {
		m_dbService = iface;
	}

	void IqrfNetworkEnum::detachInterface(IIqrfDb *iface) {
		if (m_dbService == iface) {
			m_dbService = nullptr;
		}
	}

  void IqrfNetworkEnum::attachInterface(IIqrfDpaService *iface) {
		m_dpaService = iface;
	}

	void IqrfNetworkEnum::detachInterface(IIqrfDpaService *iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void IqrfNetworkEnum::attachInterface(IJsCacheService *iface) {
		m_cacheService = iface;
	}

	void IqrfNetworkEnum::detachInterface(IJsCacheService *iface) {
		if (m_cacheService == iface) {
			m_cacheService = nullptr;
		}
	}

	void IqrfNetworkEnum::attachInterface(IJsRenderService *iface) {
		m_renderService = iface;
	}

	void IqrfNetworkEnum::detachInterface(IJsRenderService *iface) {
		if (m_renderService == iface) {
			m_renderService = nullptr;
		}
	}

	void IqrfNetworkEnum::attachInterface(shape::ILaunchService *iface) {
		m_launchService = iface;
	}

	void IqrfNetworkEnum::detachInterface(shape::ILaunchService *iface) {
		if (m_launchService == iface) {
			m_launchService = nullptr;
		}
	}

	void IqrfNetworkEnum::attachInterface(IMessagingSplitterService *iface) {
		m_splitterService = iface;
	}

	void IqrfNetworkEnum::detachInterface(IMessagingSplitterService *iface) {
		if (m_splitterService == iface) {
			m_splitterService = nullptr;
		}
	}

	void IqrfNetworkEnum::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void IqrfNetworkEnum::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}
}