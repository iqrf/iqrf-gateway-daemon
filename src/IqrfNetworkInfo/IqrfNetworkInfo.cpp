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

#include "IqrfNetworkInfo.h"
#include "iqrf__IqrfNetworkInfo.hxx"

TRC_INIT_MODULE(iqrf::IqrfNetworkInfo)

/// iqrf namespace
namespace iqrf {

	IqrfNetworkInfo::IqrfNetworkInfo() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	IqrfNetworkInfo::~IqrfNetworkInfo() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// Component lifecycle

	void IqrfNetworkInfo::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"************************************" << std::endl <<
			"IqrfNetworkInfo instance activate" << std::endl <<
			"************************************"
		);
		modify(props);
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfNetworkInfo::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		(void)props;
		TRC_FUNCTION_LEAVE("");
	}

	void IqrfNetworkInfo::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "IqrfNetworkInfo instance deactivate" << std::endl
			<< "******************************"
		);
		TRC_FUNCTION_LEAVE("");
	}

	///// Interfaces

	void IqrfNetworkInfo::attachInterface(IIqrfDpaService *iface) {
		m_dpaService = iface;
	}

	void IqrfNetworkInfo::detachInterface(IIqrfDpaService *iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void IqrfNetworkInfo::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void IqrfNetworkInfo::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}

	///// Private methods

	void IqrfNetworkInfo::handleTransactionErrors(std::unique_ptr<IDpaTransactionResult2> &result, const std::string &errorStr) {
		m_errorCode = result->getErrorCode();
		m_transactionResults.push_back(std::move(result));
		THROW_EXC(std::logic_error, errorStr);
	}

	std::set<uint8_t> IqrfNetworkInfo::getBondedDevices(const uint8_t &repeat) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> transactionResult;
		try {
			// Build DPA request
			DpaMessage bondedDevicesRequest;
			DpaMessage::DpaPacket_t bondedDevicesPacket;
			bondedDevicesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			bondedDevicesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
			bondedDevicesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
			bondedDevicesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			bondedDevicesRequest.DataToBuffer(bondedDevicesPacket.Buffer, sizeof(TDpaIFaceHeader));
			// Execute DPA request
			TRC_DEBUG("Sending CMD_COORDINATOR_BONDED_DEVICES request.");
			m_exclusiveAccess->executeDpaTransactionRepeat(bondedDevicesRequest, transactionResult, repeat);
			DpaMessage bondedDevicesResponse = transactionResult->getResponse();
			TRC_INFORMATION("CMD_COORDINATOR_BONDED_DEVICES successful.");
			// Process DPA response
			const unsigned char *pData = bondedDevicesResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
			std::set<uint8_t> bondedDevices;
			for (uint8_t addr = 1; addr <= MAX_ADDRESS; addr++) {
				if ((pData[addr / 8] & (1 << (addr % 8))) != 0) {
					bondedDevices.insert(addr);
				}
			};
			m_transactionResults.push_back(std::move(transactionResult));
			TRC_FUNCTION_LEAVE("");
			return bondedDevices;
		} catch (const std::exception &e) {
			handleTransactionErrors(transactionResult, e.what());
		}
	}

	std::set<uint8_t> IqrfNetworkInfo::getDiscoveredDevices(const std::set<uint8_t> &nodes, const uint8_t &repeat) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> transactionResult;
		try {
			// Build DPA request
			DpaMessage discoveredDevicesRequest;
			DpaMessage::DpaPacket_t discoveredDevicesPacket;
			discoveredDevicesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			discoveredDevicesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
			discoveredDevicesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_DISCOVERED_DEVICES;
			discoveredDevicesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			discoveredDevicesRequest.DataToBuffer(discoveredDevicesPacket.Buffer, sizeof(TDpaIFaceHeader));
			// Execute DPA request
			TRC_DEBUG("Sending CMD_COORDINATOR_DISCOVERED_DEVICES request.");
			m_exclusiveAccess->executeDpaTransactionRepeat(discoveredDevicesRequest, transactionResult, repeat);
			DpaMessage discoveredDevicesResponse = transactionResult->getResponse();
			TRC_INFORMATION("CMD_COORDINATOR_DISCOVERED_DEVICES successful.");
			// Process DPA response
			const unsigned char *pData = discoveredDevicesResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
			std::set<uint8_t> discoveredDevices;
			for (auto &addr : nodes) {
				if ((pData[addr / 8] & (1 << (addr % 8))) != 0) {
					discoveredDevices.insert(addr);
				}
			}
			m_transactionResults.push_back(std::move(transactionResult));
			TRC_FUNCTION_LEAVE("");
			return discoveredDevices;
		} catch (const std::exception &e) {
			handleTransactionErrors(transactionResult, e.what());
		}
	}

	std::map<uint8_t, uint32_t> IqrfNetworkInfo::getMids(const std::set<uint8_t> &nodes, const uint8_t &repeat) {
		TRC_FUNCTION_ENTER("");
		const uint16_t totalData = (*nodes.rbegin() + 1) * 8;
		const uint8_t requests = totalData / EEEPROM_READ_MAX_LEN;
		const uint8_t remainder = totalData % EEEPROM_READ_MAX_LEN;
		// Get MID data
		std::vector<uint8_t> midData;
		for (uint8_t i = 0; i < requests + 1; i++) {
			uint8_t length = i < requests ? EEEPROM_READ_MAX_LEN : remainder;
			if (length == 0) {
				break;
			}
			uint16_t address = MID_START_ADDR + i * EEEPROM_READ_MAX_LEN;
			eeepromRead(midData, address, length, repeat);
		}
		// Process MID data
		std::map<uint8_t, uint32_t> mids;
		for (auto &addr : nodes) {
			uint16_t idx = addr * 8;
			uint32_t mid = ((uint32_t)midData[idx] | ((uint32_t)midData[idx + 1] << 8) | ((uint32_t)midData[idx + 2] << 16) | ((uint32_t)midData[idx + 3] << 24));
			mids.insert(std::make_pair(addr, mid));
		}
		TRC_FUNCTION_LEAVE("");
		return mids;
	}

	std::map<uint8_t, uint8_t> IqrfNetworkInfo::getVrns(const std::set<uint8_t> &nodes, const uint8_t &repeat) {
		TRC_FUNCTION_ENTER("");
		const uint16_t totalData = *nodes.rbegin() + 1;
		const uint8_t requests = totalData / EEEPROM_READ_MAX_LEN;
		const uint8_t remainder = totalData % EEEPROM_READ_MAX_LEN;
		// Get VRN data
		std::vector<uint8_t> vrnData;
		for (uint8_t i = 0; i < requests + 1; i++) {
			uint8_t length = i < requests ? EEEPROM_READ_MAX_LEN : remainder;
			if (length == 0) {
				break;
			}
			uint16_t address = VRN_START_ADDR + i * EEEPROM_READ_MAX_LEN;
			eeepromRead(vrnData, address, length, repeat);
		}
		// Process VRN data
		std::map<uint8_t, uint8_t> vrns;
		for (auto &addr : nodes) {
			vrns.insert(std::make_pair(addr, vrnData[addr]));
		}
		TRC_FUNCTION_LEAVE("");
		return vrns;
	}

	std::map<uint8_t, uint8_t> IqrfNetworkInfo::getZones(const std::set<uint8_t> &nodes, const uint8_t &repeat) {
		TRC_FUNCTION_ENTER("");
		const uint16_t totalData = *nodes.rbegin() + 1;
		const uint8_t requests = totalData / EEEPROM_READ_MAX_LEN;
		const uint8_t remainder = totalData % EEEPROM_READ_MAX_LEN;
		// Get Zone data
		std::vector<uint8_t> zoneData;
		for (uint8_t i = 0; i < requests + 1; i++) {
			uint8_t length = i < requests ? EEEPROM_READ_MAX_LEN : remainder;
			if (length == 0) {
				break;
			}
			uint16_t address = ZONE_START_ADDR + i * EEEPROM_READ_MAX_LEN;
			eeepromRead(zoneData, address, length, repeat);
		}
		// Process Zone data
		std::map<uint8_t, uint8_t> zones;
		for (auto &addr : nodes) {
			zones.insert(std::make_pair(addr, zoneData[addr]));
		}
		TRC_FUNCTION_LEAVE("");
		return zones;
	}

	std::map<uint8_t, uint8_t> IqrfNetworkInfo::getParents(const std::set<uint8_t> &nodes, const uint8_t &repeat) {
		TRC_FUNCTION_ENTER("");
		const uint16_t totalData = *nodes.rbegin() + 1;
		const uint8_t requests = totalData / EEEPROM_READ_MAX_LEN;
		const uint8_t remainder = totalData % EEEPROM_READ_MAX_LEN;
		// Get Parent data
		std::vector<uint8_t> parentData;
		for (uint8_t i = 0; i < requests + 1; i++) {
			uint8_t length = i < requests ? EEEPROM_READ_MAX_LEN : remainder;
			if (length == 0) {
				break;
			}
			uint16_t address = PARENT_START_ADDR + i * EEEPROM_READ_MAX_LEN;
			eeepromRead(parentData, address, length, repeat);
		}
		// Process Parent data
		std::map<uint8_t, uint8_t> parents;
		for (auto &addr : nodes) {
			parents.insert(std::make_pair(addr, parentData[addr]));
		}
		TRC_FUNCTION_LEAVE("");
		return parents;
	}

	void IqrfNetworkInfo::eeepromRead(std::vector<uint8_t> &data, const uint16_t &address, const uint8_t &length, const uint8_t &repeat) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> transactionResult;
		try {
			// Build DPA request
			DpaMessage eeepromReadRequest;
			DpaMessage::DpaPacket_t eeepromReadPacket;
			eeepromReadPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			eeepromReadPacket.DpaRequestPacket_t.PNUM = PNUM_EEEPROM;
			eeepromReadPacket.DpaRequestPacket_t.PCMD = CMD_EEEPROM_XREAD;
			eeepromReadPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			eeepromReadPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.Address = address;
			eeepromReadPacket.DpaRequestPacket_t.DpaMessage.XMemoryRequest.ReadWrite.Read.Length = length;
			eeepromReadRequest.DataToBuffer(eeepromReadPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(uint16_t) + sizeof(uint8_t));
			// Execute DPA request
			TRC_DEBUG("Sending CMD_EEEPROM_XREAD request.");
			m_exclusiveAccess->executeDpaTransactionRepeat(eeepromReadRequest, transactionResult, repeat);
			DpaMessage eepromReadResponse = transactionResult->getResponse();
			TRC_INFORMATION("CMD_EEEPROM_XREAD successful.");
			// Process DPA response
			const uint8_t *pData = eepromReadResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
			for (uint8_t i = 0; i < length; i++) {
				data.push_back(pData[i]);
			}
			m_transactionResults.push_back(std::move(transactionResult));
			TRC_FUNCTION_ENTER("");
		} catch (const std::exception &e) {
			handleTransactionErrors(transactionResult, e.what());
		}
	}

	///// API

	void IqrfNetworkInfo::getNetworkInfo(NetworkInfoResult &result, const uint8_t &repeat) {
		TRC_FUNCTION_ENTER("");
		std::lock_guard<std::mutex> lock(m_mtx);
		m_errorCode = 0;

		// Exclusive access
		try {
			m_exclusiveAccess = m_dpaService->getExclusiveAccess();
		} catch (const std::exception &e) {
			m_errorCode = 1002;
			THROW_EXC(std::logic_error, e.what());
		}

		// Execute
		try {
			m_transactionResults.clear();
			std::set<uint8_t> bondedDevices = getBondedDevices(repeat);
			if (bondedDevices.size() == 0) {
				m_errorCode = 1003;
				THROW_EXC(std::logic_error, "No node devices in network.");
			}
			result.setBondedDevices(bondedDevices);
			std::set<uint8_t> discoveredDevices = getDiscoveredDevices(bondedDevices, repeat);
			result.setDiscoveredDevices(discoveredDevices);
			if (result.getRetrieveMids()) {
				std::map<uint8_t, uint32_t> mids = getMids(bondedDevices, repeat);
				result.setMids(mids);
			}
			if (result.getRetrieveVrns()) {
				std::map<uint8_t, uint8_t> vrns = getVrns(discoveredDevices, repeat);
				result.setVrns(vrns);
			}
			if (result.getRetrieveZones()) {
				std::map<uint8_t, uint8_t> zones = getZones(discoveredDevices, repeat);
				result.setZones(zones);
			}
			if (result.getRetrieveParents()) {
				std::map<uint8_t, uint8_t> parents = getParents(discoveredDevices, repeat);
				result.setParents(parents);
			}
			m_exclusiveAccess.reset();
		} catch (const std::exception &e) {
			m_exclusiveAccess.reset();
			THROW_EXC(std::logic_error, e.what());
		}
	}

	std::list<std::unique_ptr<IDpaTransactionResult2>> IqrfNetworkInfo::getTransactionResults() {
		std::list<std::unique_ptr<IDpaTransactionResult2>> transactionResults;
		std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator itr;
		for (itr = m_transactionResults.begin(); itr != m_transactionResults.end(); itr++) {
			transactionResults.push_back(std::move(*itr));
		}
		return transactionResults;
	}

	int IqrfNetworkInfo::getErrorCode() {
		return m_errorCode;
	}
}
