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

#include "DpaValueService.h"
#include "iqrf__DpaValueService.hxx"

TRC_INIT_MODULE(iqrf::DpaValueService)

/// iqrf namespace
namespace iqrf {

	DpaValueService::DpaValueService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	DpaValueService::~DpaValueService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// Auxiliary methods

	void DpaValueService::setErrorTransactionResult(DpaValueResult &serviceResult, std::unique_ptr<IDpaTransactionResult2> &result, const std::string &errorStr) {
		serviceResult.setStatus(result->getErrorCode(), errorStr);
		serviceResult.addTransactionResult(result);
		THROW_EXC(std::logic_error, errorStr);
	}

	///// Message handling

	TDpaValueType DpaValueService::setDpaValueType(DpaValueResult &serviceResult, TDpaValueType type) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		TDpaValueType previousType;
		try {
			// Prepare DPA request
			DpaMessage dpaParamRequest;
			DpaMessage::DpaPacket_t dpaParamPacket;
			dpaParamPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			dpaParamPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
			dpaParamPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SET_DPAPARAMS;
			dpaParamPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			dpaParamPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetDpaParams_Request_Response.DpaParam = (uint8_t)type;
			dpaParamRequest.DataToBuffer(dpaParamPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerCoordinatorSetDpaParams_Request_Response));
			// Execute DPA transaction
			TRC_DEBUG("Sending CMD_COORDINATOR_SET_DPAPARAMS request.");
			m_exclusiveAccess->executeDpaTransactionRepeat(dpaParamRequest, result, m_requestParams.repeat);
			// Get DPA response
			DpaMessage dpaParamResponse = result->getResponse();
			TRC_INFORMATION("CMD_COORDINATOR_SET_DPAPARAMS successful.");
			// Parse DPA response
			serviceResult.addTransactionResult(result);
			previousType = (TDpaValueType)dpaParamResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorSetDpaParams_Request_Response.DpaParam;
		} catch (const std::exception &e) {
			setErrorTransactionResult(serviceResult, result, e.what());
		}
		TRC_FUNCTION_LEAVE("");
		return previousType;
	}

	void DpaValueService::handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, Document doc) {
		TRC_FUNCTION_ENTER(PAR(messagingId) <<
			NAME_PAR(mType, msgType.m_type) <<
			NAME_PAR(major, msgType.m_major) <<
			NAME_PAR(minor, msgType.m_minor) <<
			NAME_PAR(patch, msgType.m_micro)
		);

		ComIqmeshNetworkDpaValue params(doc);
		m_requestParams = params.getDpaValueParams();
		DpaValueResult result;
		result.setMessageType(msgType.m_type);
		result.setMessageId(params.getMsgId());
		result.setVerbose(params.getVerbose());
		result.setAction(m_requestParams.action);
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
			if (m_requestParams.action == TDpaParamAction::GET) {
				TDpaValueType defaultType = TDpaValueType::DpaValueType_RSSI, current;
				current = setDpaValueType(result, defaultType);
				result.setValueType(current);
				if (current != defaultType) {
					setDpaValueType(result, current);
				}
			} else {
				result.setValueType(m_requestParams.type);
				setDpaValueType(result, m_requestParams.type);
			}
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

	void DpaValueService::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "DpaValueService instance activate" << std::endl
			<< "******************************"
		);
		modify(props);
		m_splitterService->registerFilteredMsgHandler(
			m_mTypes, [&](const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, Document doc) {
				handleMsg(messagingId, msgType, std::move(doc));
			}
		);
		TRC_FUNCTION_LEAVE("");
	}

	void DpaValueService::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		(void)props;
		TRC_FUNCTION_LEAVE("");
	}

	void DpaValueService::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "DpaValueService instance deactivate" << std::endl
			<< "******************************"
		);
		m_splitterService->unregisterFilteredMsgHandler(m_mTypes);
		TRC_FUNCTION_LEAVE("");
	}

	///// Interface management

	void DpaValueService::attachInterface(IIqrfDpaService *iface) {
		m_dpaService = iface;
	}

	void DpaValueService::detachInterface(IIqrfDpaService *iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void DpaValueService::attachInterface(IMessagingSplitterService *iface) {
		m_splitterService = iface;
	}

	void DpaValueService::detachInterface(IMessagingSplitterService *iface) {
		if (m_splitterService == iface) {
			m_splitterService = nullptr;
		}
	}

	void DpaValueService::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void DpaValueService::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}
}
