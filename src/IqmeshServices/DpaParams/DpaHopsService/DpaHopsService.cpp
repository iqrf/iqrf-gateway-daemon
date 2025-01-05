/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#include "DpaHopsService.h"
#include "iqrf__DpaHopsService.hxx"

TRC_INIT_MODULE(iqrf::DpaHopsService)

/// iqrf namespace
namespace iqrf {

    DpaHopsService::DpaHopsService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	DpaHopsService::~DpaHopsService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// Auxiliary methods

	void DpaHopsService::setErrorTransactionResult(DpaHopsResult &serviceResult, std::unique_ptr<IDpaTransactionResult2> &result, const std::string &errorStr) {
		serviceResult.setStatus(result->getErrorCode(), errorStr);
		serviceResult.addTransactionResult(result);
		THROW_EXC(std::logic_error, errorStr);
	}

	///// Message handling

	std::tuple<uint8_t, uint8_t> DpaHopsService::setDpaHops(DpaHopsResult &serviceResult, std::tuple<uint8_t, uint8_t> &hops) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		std::tuple<uint8_t, uint8_t> previousHops;
		try {
			// Prepare DPA request
			DpaMessage setHopsRequest;
			DpaMessage::DpaPacket_t setHopsPacket;
			setHopsPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			setHopsPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
			setHopsPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_SET_HOPS;
			setHopsPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			setHopsPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetHops_Request_Response.RequestHops = std::get<0>(hops);
			setHopsPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorSetHops_Request_Response.ResponseHops = std::get<1>(hops);
			setHopsRequest.DataToBuffer(setHopsPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerCoordinatorSetHops_Request_Response));
			// Execute DPA transaction
			TRC_DEBUG("Sending CMD_COORDINATOR_SET_HOPS request.");
			m_exclusiveAccess->executeDpaTransactionRepeat(setHopsRequest, result, m_requestParams.repeat);
			// Get DPA response
			DpaMessage setHopsResponse = result->getResponse();
			TRC_INFORMATION("CMD_COORDINATOR_SET_HOPS successful.");
			// Parse DPA response
			serviceResult.addTransactionResult(result);
			uint8_t requestHops = setHopsResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorSetHops_Request_Response.RequestHops;
			uint8_t responseHops = setHopsResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorSetHops_Request_Response.ResponseHops;
			previousHops = std::make_tuple(requestHops, responseHops);
		} catch (const std::exception &e) {
			setErrorTransactionResult(serviceResult, result, e.what());
		}
		TRC_FUNCTION_LEAVE("");
		return previousHops;
	}

	void DpaHopsService::handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, Document doc) {
		TRC_FUNCTION_ENTER(PAR(messagingId) <<
			NAME_PAR(mType, msgType.m_type) <<
			NAME_PAR(major, msgType.m_major) <<
			NAME_PAR(minor, msgType.m_minor) <<
			NAME_PAR(patch, msgType.m_micro)
		);

		ComIqmeshNetworkDpaHops params(doc);
		m_requestParams = params.getDpaHopsParams();
		DpaHopsResult result;
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
				std::tuple<uint8_t, uint8_t> defaultHops = {0, 0}, currentHops;
				currentHops = setDpaHops(result, defaultHops);
				result.setRequestHops(std::get<0>(currentHops));
				result.setResponseHops(std::get<1>(currentHops));
				if (currentHops != defaultHops) {
					setDpaHops(result, currentHops);
				}
			} else {
				std::tuple<uint8_t, uint8_t> hops = {m_requestParams.requestHops, m_requestParams.responseHops};
				result.setRequestHops(m_requestParams.requestHops);
				result.setResponseHops(m_requestParams.responseHops);
				setDpaHops(result, hops);
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

	void DpaHopsService::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "DpaHopsService instance activate" << std::endl
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

	void DpaHopsService::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		(void)props;
		TRC_FUNCTION_LEAVE("");
	}

	void DpaHopsService::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "DpaHopsService instance deactivate" << std::endl
			<< "******************************"
		);
		m_splitterService->unregisterFilteredMsgHandler(m_mTypes);
		TRC_FUNCTION_LEAVE("");
	}

	///// Interface management

	void DpaHopsService::attachInterface(IIqrfDpaService *iface) {
		m_dpaService = iface;
	}

	void DpaHopsService::detachInterface(IIqrfDpaService *iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void DpaHopsService::attachInterface(IMessagingSplitterService *iface) {
		m_splitterService = iface;
	}

	void DpaHopsService::detachInterface(IMessagingSplitterService *iface) {
		if (m_splitterService == iface) {
			m_splitterService = nullptr;
		}
	}

	void DpaHopsService::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void DpaHopsService::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}
}

