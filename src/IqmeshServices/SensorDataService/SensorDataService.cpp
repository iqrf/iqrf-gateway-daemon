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

	///// Message handling

	void SensorDataService::readSensorData(SensorDataResult &result) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> transResult;
		try {
			std::set<uint8_t>::iterator it;
			for (it = m_params.devices.begin(); it != m_params.devices.end(); ++it) {
				const uint8_t addr = *it;
				rapidjson::Document params(kObjectType);
				Pointer("/sensorIndexes").Set(params, -1);
				sensor::jsdriver::ReadSensorsWithTypes readSensors(m_jsRenderService, addr, params);
				readSensors.processRequestDrv();
				m_dpaService->executeDpaTransactionRepeat(readSensors.getRequest(), transResult, m_params.repeat);
				readSensors.processDpaTransactionResult(std::move(transResult));
				try {
					uint32_t mid = m_dbService->getDeviceMid(addr);
					result.addDeviceMid(addr, mid);
				} catch (const std::exception &e) {
					result.addDeviceMid(addr, 0);
				}
				result.addSensorData(addr, readSensors.getSensors());
			}
		} catch (const std::exception &e) {
			setErrorTransactionResult(result, transResult, e.what());
		}
		TRC_FUNCTION_LEAVE("");
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
			readSensorData(result);
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
