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

#include "SensorDataService.h"
#include "iqrf__SensorDataService.hxx"

namespace iqrf {

	SensorDataService::SensorDataService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	SensorDataService::~SensorDataService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// Auxiliary methods

	void SensorDataService::setErrorTransactionResult(SensorDataResult &result, std::unique_ptr<IDpaTransactionResult2> &transResult, const std::string &errorStr) {
		result.setStatus(transResult->getErrorCode(), errorStr);
		result.addTransactionResult(transResult);
		THROW_EXC(std::logic_error, errorStr);
	}

	uint8_t SensorDataService::frcDeviceCountByType(const uint8_t &type) {
		if (type >= 0x01 && type <= 0x7F) {
			return 31;
		}
		if (type >= 0x80 && type <= 0x9F) {
			return 61;
		}
		if (type >= 0xA0 && type <= 0xBF) {
			return 15;
		}
		throw std::domain_error("Unknown or unsupported sensor type.");
	}

	bool SensorDataService::extraResultRequired(const uint8_t &command, const uint8_t &deviceCount) {
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

	///// Message handling

	void SensorDataService::setOfflineFrc(SensorDataResult &result) {
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
			m_exclusiveAccess->executeDpaTransactionRepeat(setOfflineFrcRequest, transResult, m_params.repeat);
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

	void SensorDataService::sendSensorFrc(SensorDataResult &result, const uint8_t &type, const uint8_t &idx, std::set<uint8_t> &nodes) {
		uint8_t command = getSensorFrcCommand(type);
		std::unique_ptr<IDpaTransactionResult2> transResult;
		try {
			sensor::jsdriver::SensorFrcJs sensorFrc(m_jsRenderService, type, idx, command, nodes);
			sensorFrc.processRequestDrv();
			// frc send selective
			m_dpaService->executeDpaTransactionRepeat(sensorFrc.getFrcRequest(), transResult, m_params.repeat);
			sensorFrc.setFrcDpaTransactionResult(std::move(transResult));
			// frc extra result
			if (extraResultRequired(command, nodes.size())) {
				m_dpaService->executeDpaTransactionRepeat(sensorFrc.getFrcExtraRequest(), transResult, m_params.repeat);
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
				const uint8_t addr = sensor->getAddr();
				try {
					uint32_t mid = m_dbService->getDeviceMid(addr);
					result.addDeviceMid(addr, mid);
				} catch (const std::exception &e) {
					result.addDeviceMid(addr, 0);
				}
				sensor->setIdx(m_dbService->getGlobalSensorIndex(sensor->getAddr(), sensor->getType(), sensor->getIdx()));
			}
			result.addSensorData(sensorFrc.getSensors());
		} catch (const std::exception &e) {
			setErrorTransactionResult(result, transResult, e.what());
		}
	}

	void SensorDataService::getTypeData(SensorDataResult &result, const uint8_t &type, const uint8_t &idx, std::deque<uint8_t> &addresses) {
		const uint8_t devicesPerRequest = frcDeviceCountByType(type);
		const uint8_t v = addresses.size() / devicesPerRequest;
		const uint8_t r = addresses.size() % devicesPerRequest;
		std::vector<std::set<uint8_t>> vectors;
		for (uint8_t i = 0; i < v; ++i) {
			vectors.push_back(std::set<uint8_t>(addresses.begin(), addresses.begin() + devicesPerRequest));
			addresses.erase(addresses.begin(), addresses.begin() + devicesPerRequest);
		}
		vectors.push_back(std::set<uint8_t>(addresses.begin(), addresses.end()));
		for (auto &vector : vectors) {
			setOfflineFrc(result);
			sendSensorFrc(result, type, idx, vector);
		}
	}

	void SensorDataService::getDataByFrc(SensorDataResult &result) {
		SensorSelectMap map = m_dbService->constructSensorSelectMap();
		for (const auto& [type, addrIdx] : map) {
			TRC_DEBUG("type: " << std::to_string(type));
			if (type >= 0xC0) {
				continue;
			}
			for (uint8_t desiredIdx = 0; desiredIdx < 32; ++desiredIdx) {
				std::deque<uint8_t> addrs;
				for (const auto& [addr, idx] : addrIdx) {
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


	void SensorDataService::handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, Document doc) {
		TRC_FUNCTION_ENTER(
			PAR(messagingId) <<
			NAME_PAR(mType, msgType.m_type) <<
			NAME_PAR(major, msgType.m_major) <<
			NAME_PAR(minor, msgType.m_minor) <<
			NAME_PAR(patch, msgType.m_micro)
		);

		SensorDataParams params(doc);
		m_params = params.getInputParams();
		SensorDataResult result;
		result.setMessageType(msgType.m_type);
		result.setMessageId(params.getMsgId());
		result.setVerbose(params.getVerbose());
		Document response;

		try {
			m_exclusiveAccess = m_dpaService->getExclusiveAccess();
		} catch (const std::exception &e) {
			CATCH_EXC_TRC_WAR(std::exception, e, "Failed to acquire exclusive access: " << e.what());
			result.setStatus(ErrorCodes::exclusiveAccessError, e.what());
			result.createErrorResponse(response);
			m_splitterService->sendMessage(messagingId, std::move(response));
			TRC_FUNCTION_LEAVE("");
			return;
		}

		try {
			getDataByFrc(result);
			m_dbService->updateSensorValues(result.getSensorData());
		} catch (const std::exception &e) {
			CATCH_EXC_TRC_WAR(std::exception, e, e.what());
		}

		m_exclusiveAccess.reset();

		// Create and send response
		result.createResponse(response);
		m_splitterService->sendMessage(messagingId, std::move(response));

		TRC_FUNCTION_LEAVE("");
	}

	///// Component management

	void SensorDataService::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "SensorDataService instance activate" << std::endl
			<< "******************************"
		);
		modify(props);
		m_splitterService->registerFilteredMsgHandler(
			m_mTypes,
			[&](const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, Document doc) {
				handleMsg(messagingId, msgType, std::move(doc));
			}
		);
		TRC_FUNCTION_LEAVE("");
	}

	void SensorDataService::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		(void)props;
		TRC_FUNCTION_LEAVE("");
	}

	void SensorDataService::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "SensorDataService instance deactivate" << std::endl
			<< "******************************"
		);
		TRC_FUNCTION_LEAVE("");
	}

	///// Interface management

	void SensorDataService::attachInterface(IIqrfDb *iface) {
		m_dbService = iface;
	}

	void SensorDataService::detachInterface(IIqrfDb *iface) {
		if (m_dbService == iface) {
			m_dbService = nullptr;
		}
	}

	void SensorDataService::attachInterface(IIqrfDpaService *iface) {
		m_dpaService = iface;
	}

	void SensorDataService::detachInterface(IIqrfDpaService *iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void SensorDataService::attachInterface(IJsRenderService *iface) {
		m_jsRenderService = iface;
	}

	void SensorDataService::detachInterface(IJsRenderService *iface) {
		if (m_jsRenderService == iface) {
			m_jsRenderService = nullptr;
		}
	}

	void SensorDataService::attachInterface(IMessagingSplitterService *iface) {
		m_splitterService = iface;
	}

	void SensorDataService::detachInterface(IMessagingSplitterService *iface) {
		if (m_splitterService == iface) {
			m_splitterService = nullptr;
		}
	}

	void SensorDataService::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void SensorDataService::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}
}
