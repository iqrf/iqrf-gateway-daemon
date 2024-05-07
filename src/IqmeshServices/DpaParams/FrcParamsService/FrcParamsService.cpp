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

#include "FrcParamsService.h"
#include "iqrf__FrcParamsService.hxx"

TRC_INIT_MODULE(iqrf::DpaHopsService)

/// iqrf namespace
namespace iqrf {

    FrcParamsService::FrcParamsService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	FrcParamsService::~FrcParamsService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// Auxiliary methods

	void FrcParamsService::setErrorTransactionResult(FrcParamsResult &serviceResult, std::unique_ptr<IDpaTransactionResult2> &result, const std::string &errorStr) {
		serviceResult.setStatus(result->getErrorCode(), errorStr);
		serviceResult.addTransactionResult(result);
		THROW_EXC(std::logic_error, errorStr);
	}

	///// Message handling

	uint8_t FrcParamsService::setFrcResponseTime(FrcParamsResult &serviceResult, const uint8_t &param) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		uint8_t previous;
		try {
			// Prepare DPA request
			DpaMessage setFrcParamRequest;
			DpaMessage::DpaPacket_t setFrcParamPacket;
			setFrcParamPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			setFrcParamPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
			setFrcParamPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SET_PARAMS;
			setFrcParamPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			setFrcParamPacket.DpaRequestPacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams = param;
			setFrcParamRequest.DataToBuffer(setFrcParamPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerFrcSetParams_RequestResponse));
			// Execute FRC transaction
			TRC_DEBUG("Sending CMD_FRC_SET_PARAMS request.");
			m_exclusiveAccess->executeDpaTransactionRepeat(setFrcParamRequest, result, m_requestParams.repeat);
			// Get DPA response
			DpaMessage setFrcParamResponse = result->getResponse();
			TRC_INFORMATION("CMD_FRC_SET_PARAMS successful.");
			// Parse DPA response
			serviceResult.addTransactionResult(result);
			previous = setFrcParamResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams;
		} catch (const std::exception &e) {
			setErrorTransactionResult(serviceResult, result, e.what());
		}
		TRC_FUNCTION_LEAVE("");
		return previous;
	}

	void FrcParamsService::handleMsg(const MessagingInstance &messaging, const IMessagingSplitterService::MsgType &msgType, Document doc) {
		TRC_FUNCTION_ENTER(
			PAR( messaging.to_string() ) <<
			NAME_PAR(mType, msgType.m_type) <<
			NAME_PAR(major, msgType.m_major) <<
			NAME_PAR(minor, msgType.m_minor) <<
			NAME_PAR(patch, msgType.m_micro)
		);

		ComIqmeshNetworkFrcParams params(doc);
		m_requestParams = params.getFrcParams();
		FrcParamsResult result;
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
			m_splitterService->sendMessage(messaging, std::move(response));
			TRC_FUNCTION_LEAVE("");
			return;
		}

		try {
			if (m_requestParams.action == TDpaParamAction::GET) {
				uint8_t defaultParam = (uint8_t)IDpaTransaction2::FrcResponseTime::k40Ms, currentParam;
				currentParam = setFrcResponseTime(result, defaultParam);
				result.setResponseTime((IDpaTransaction2::FrcResponseTime)(currentParam & FRC_RESPONSE_TIME_MASK));
				result.setOfflineFrc(currentParam & FRC_OFFLINE_FRC_MASK);
				if (currentParam != defaultParam) {
					setFrcResponseTime(result, currentParam);
				}
			} else {
				uint8_t param = (uint8_t)m_requestParams.responseTime;
				if (m_requestParams.offlineFrc) {
					param |= FRC_OFFLINE_FRC_MASK;
				}
				result.setResponseTime(m_requestParams.responseTime);
				result.setOfflineFrc(m_requestParams.offlineFrc);
				setFrcResponseTime(result, param);
			}
		} catch (const std::exception &e) {
			CATCH_EXC_TRC_WAR(std::exception, e, e.what());
		}

		m_exclusiveAccess.reset();

		// Create and send response
		result.createResponse(response);
		m_splitterService->sendMessage(messaging, std::move(response));

		TRC_FUNCTION_LEAVE("");
	}

	///// Component management

	void FrcParamsService::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "FrcParamsService instance activate" << std::endl
			<< "******************************"
		);
		modify(props);
		m_splitterService->registerFilteredMsgHandler(
			m_mTypes,
			[&](const MessagingInstance &messaging, const IMessagingSplitterService::MsgType &msgType, Document doc) {
				handleMsg(messaging, msgType, std::move(doc));
			}
		);
		TRC_FUNCTION_LEAVE("");
	}

	void FrcParamsService::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		(void)props;
		TRC_FUNCTION_LEAVE("");
	}

	void FrcParamsService::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "FrcParamsService instance deactivate" << std::endl
			<< "******************************"
		);
		m_splitterService->unregisterFilteredMsgHandler(m_mTypes);
		TRC_FUNCTION_LEAVE("");
	}

	///// Interface management

	void FrcParamsService::attachInterface(IIqrfDpaService *iface) {
		m_dpaService = iface;
	}

	void FrcParamsService::detachInterface(IIqrfDpaService *iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void FrcParamsService::attachInterface(IMessagingSplitterService *iface) {
		m_splitterService = iface;
	}

	void FrcParamsService::detachInterface(IMessagingSplitterService *iface) {
		if (m_splitterService == iface) {
			m_splitterService = nullptr;
		}
	}

	void FrcParamsService::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void FrcParamsService::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}
}

