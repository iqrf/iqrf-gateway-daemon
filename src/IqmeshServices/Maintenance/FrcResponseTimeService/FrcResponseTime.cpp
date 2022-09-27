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

#include "FrcResponseTime.h"
#include "iqrf__FrcResponseTime.hxx"

TRC_INIT_MODULE(iqrf::FrcResponseTime)

/// iqrf namespace
namespace iqrf {

	FrcResponseTime::FrcResponseTime() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	FrcResponseTime::~FrcResponseTime() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// Auxiliary methods

	std::set<uint8_t> FrcResponseTime::nodeBitmapToSet(const uint8_t *bitmap) {
		std::set<uint8_t> nodes;
		for (uint8_t i = 0; i <= MAX_ADDRESS; i++) {
			if (bitmap[i / 8] & (1 << (i % 8))) {
				nodes.insert(i);
			}
		}
		return nodes;
	}

	std::vector<uint8_t> FrcResponseTime::selectNodes(const std::set<uint8_t> &nodes, uint8_t &idx, const uint8_t &count) {
		std::vector<uint8_t> selectedNodes(30, 0);
		std::set<uint8_t>::iterator itr = nodes.begin();
		std::advance(itr, idx);
		for (std::set<uint8_t>::iterator end = std::next(itr, count); itr != end; ++itr) {
			selectedNodes[*itr / 8] |= (1 << (*itr % 8));
			idx++;
		}
		return selectedNodes;
	}

	void FrcResponseTime::setErrorTransactionResult(FrcResponseTimeResult &serviceResult, std::unique_ptr<IDpaTransactionResult2> &result, const std::string &errorStr) {
		serviceResult.setStatus(result->getErrorCode(), errorStr);
		serviceResult.addTransactionResult(result);
		THROW_EXC(std::logic_error, errorStr);
	}

	///// Message handling

	void FrcResponseTime::getBondedNodes(FrcResponseTimeResult &serviceResult) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		try {
			// Prepare DPA request
			DpaMessage bondedRequest;
			DpaMessage::DpaPacket_t bondedPacket;
			bondedPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			bondedPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
			bondedPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
			bondedPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			bondedRequest.DataToBuffer(bondedPacket.Buffer, sizeof(TDpaIFaceHeader));
			// Execute DPA transaction
			TRC_DEBUG("Sending CMD_COORDINATOR_BONDED_DEVICES request.");
			m_exclusiveAccess->executeDpaTransactionRepeat(bondedRequest, result, m_requestParams.repeat);
			// Get DPA response
			DpaMessage bondedResponse = result->getResponse();
			TRC_INFORMATION("CMD_COORDINATOR_BONDED_DEVICES successful.");
			// Parse DPA response
			serviceResult.addTransactionResult(result);
			std::set<uint8_t> bondedNodes = nodeBitmapToSet(bondedResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData);
			serviceResult.setBondedNodes(bondedNodes);
		} catch (const std::exception &e) {
			setErrorTransactionResult(serviceResult, result, e.what());
		}
		TRC_FUNCTION_LEAVE("");
	}

	IDpaTransaction2::FrcResponseTime FrcResponseTime::setFrcResponseTime(FrcResponseTimeResult &serviceResult, IDpaTransaction2::FrcResponseTime responseTime) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		IDpaTransaction2::FrcResponseTime previousResponseTime;
		try {
			// Prepare DPA request
			DpaMessage setFrcParamRequest;
			DpaMessage::DpaPacket_t setFrcParamPacket;
			setFrcParamPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			setFrcParamPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
			setFrcParamPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SET_PARAMS;
			setFrcParamPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			setFrcParamPacket.DpaRequestPacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams = (uint8_t)responseTime;
			setFrcParamRequest.DataToBuffer(setFrcParamPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerFrcSetParams_RequestResponse));
			// Execute DPA transaction
			TRC_DEBUG("Sending CMD_FRC_SET_PARAMS request.");
			m_exclusiveAccess->executeDpaTransactionRepeat(setFrcParamRequest, result, m_requestParams.repeat);
			// Get DPA response
			DpaMessage setFrcParamsResponse = result->getResponse();
			TRC_INFORMATION("CMD_FRC_SET_PARAMS successful.");
			// Parse DPA response
			serviceResult.addTransactionResult(result);
			previousResponseTime = (IDpaTransaction2::FrcResponseTime)setFrcParamsResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams;
		} catch (const std::exception &e) {
			setErrorTransactionResult(serviceResult, result, e.what());
		}
		TRC_FUNCTION_LEAVE("");
		return previousResponseTime;
	}

	IDpaTransaction2::FrcResponseTime FrcResponseTime::getResponseTime(FrcResponseTimeResult &serviceResult) {
		TRC_FUNCTION_ENTER("");
		std::set<uint8_t> bonded = serviceResult.getBondedNodes();
		// Calculate FRCs
		uint8_t frcCount = std::floor(bonded.size() / FRC_1BYTE_MAX_NODES);
		uint8_t frcRemainder = bonded.size() % FRC_1BYTE_MAX_NODES;
		// Execute
		uint8_t processedNodes = 0;
		uint8_t responded = 0;
		uint8_t unhandled = 0;
		std::vector<uint8_t> frcData;
		for (uint8_t i = 0, n = frcCount; i <= n; ++i) {
			uint8_t nodes = (uint8_t)(i < frcCount ? FRC_1BYTE_MAX_NODES : frcRemainder);
			if (nodes == 0) {
				break;
			}
			frcSendSelective(serviceResult, nodes, processedNodes, responded, frcData);
			if (nodes > FRC_RESPONSE_MAX_BYTES) {
				frcExtraResult(serviceResult, nodes - FRC_RESPONSE_MAX_BYTES + 1, frcData);
			}
		}
		uint8_t recommended = 0;
		uint8_t i = 0;
		std::map<uint8_t, uint8_t> responseTimeMap;
		for (auto &addr : bonded) {
			responseTimeMap.insert(std::make_pair(addr, frcData[i]));
			if (frcData[i] == 0xFF) {
				unhandled++;
			} else if (frcData[i] > recommended) {
				recommended = frcData[i];
			}
			i++;
		}
		if (responded == 0) {
			std::string errorStr = "No node in network responded.";
			serviceResult.setStatus(noRespondedNodesError, errorStr);
			THROW_EXC(NoRespondedNodesException, errorStr);
		}
		if (unhandled == bonded.size()) {
			std::string errorStr = "No node in network handled FRC response time event.";
			serviceResult.setStatus(noHandledNodesError, errorStr);
			THROW_EXC(std::logic_error, errorStr);
		}
		serviceResult.setInaccessibleNodes(responded);
		serviceResult.setUnhandledNodes(unhandled);
		serviceResult.setResponseTimeMap(responseTimeMap);
		TRC_FUNCTION_LEAVE("");
		return (IDpaTransaction2::FrcResponseTime)(recommended - 1);
	}

	void FrcResponseTime::frcSendSelective(FrcResponseTimeResult &serviceResult, const uint8_t &count, uint8_t &processed, uint8_t &responded, std::vector<uint8_t> &data) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		try {
			// Prepare FRC request
			DpaMessage frcSendSelectiveRequest;
			DpaMessage::DpaPacket_t frcSendSelectivePacket;
			frcSendSelectivePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			frcSendSelectivePacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
			frcSendSelectivePacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
			frcSendSelectivePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			// Set FRC command and user data
			frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_FrcResponseTime;
			frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0] = m_requestParams.command;
			frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[1] = 0;
			// Set FRC selected nodes
			std::vector<uint8_t> nodes = selectNodes(serviceResult.getBondedNodes(), processed, count);
			std::copy(nodes.begin(), nodes.end(), frcSendSelectivePacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes);
			std::memset(frcSendSelectivePacket.Buffer + 39, 0, 25 * sizeof(uint8_t));
			frcSendSelectiveRequest.DataToBuffer(frcSendSelectivePacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerFrcSendSelective_Request));
			// Execute FRC request
			m_exclusiveAccess->executeDpaTransactionRepeat(frcSendSelectiveRequest, result, m_requestParams.repeat);
			DpaMessage frcSendSelectiveResponse = result->getResponse();
			// Process FRC response
			uint8_t status = frcSendSelectiveResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
			if (status > MAX_ADDRESS) {
				THROW_EXC_TRC_WAR(std::logic_error, "FRC unsuccessful.");
			}
			responded += status;
			const uint8_t *pData = frcSendSelectiveResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData;
			uint8_t toCopy = count >= FRC_RESPONSE_MAX_BYTES ? FRC_RESPONSE_MAX_BYTES : count + 1;
			data.insert(data.end(), pData + 1, pData + toCopy);
      // Add FRC result
			serviceResult.addTransactionResult(result);
		} catch (const std::exception &e) {
			setErrorTransactionResult(serviceResult, result, e.what());
		}
		TRC_FUNCTION_LEAVE("");
	}

	void FrcResponseTime::frcExtraResult(FrcResponseTimeResult &serviceResult, const uint8_t &count, std::vector<uint8_t> &data) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> result;
		try {
			// Prepare FRC request
			DpaMessage frcExtraResultRequest;
			DpaMessage::DpaPacket_t frcExtraResultPacket;
			frcExtraResultPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			frcExtraResultPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
			frcExtraResultPacket.DpaRequestPacket_t.PCMD = CMD_FRC_EXTRARESULT;
			frcExtraResultPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			frcExtraResultRequest.DataToBuffer(frcExtraResultPacket.Buffer, sizeof(TDpaIFaceHeader));
			// Execute FRC request
			m_exclusiveAccess->executeDpaTransactionRepeat(frcExtraResultRequest, result, m_requestParams.repeat);
			DpaMessage frcExtraResultResponse = result->getResponse();
			// Process FRC response
			const uint8_t *pData = frcExtraResultResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
			data.insert(data.end(), pData, pData + count);
			serviceResult.addTransactionResult(result);
		} catch (const std::exception &e) {
			setErrorTransactionResult(serviceResult, result, e.what());
		}
		TRC_FUNCTION_LEAVE("");
	}

	void FrcResponseTime::handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, Document doc) {
		TRC_FUNCTION_ENTER(PAR(messagingId) <<
			NAME_PAR(mType, msgType.m_type) <<
			NAME_PAR(major, msgType.m_major) <<
			NAME_PAR(minor, msgType.m_minor) <<
			NAME_PAR(patch, msgType.m_micro)
		);

		ComIqmeshMaintenanceFrcResponse params(doc);
		m_requestParams = params.getFrcResponseTimeParams();
		FrcResponseTimeResult result;
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
			getBondedNodes(result);
			if (result.getBondedNodes().size() == 0) {
				std::string errorStr = "There are no bonded nodes in network.";
				result.setStatus(ErrorCodes::noBondedNodesError, errorStr);
				THROW_EXC(NoBondedNodesException, errorStr);
			}
			// Set shortest FRC response time
			m_dpaService->setFrcResponseTime(IDpaTransaction2::FrcResponseTime::k40Ms);
			IDpaTransaction2::FrcResponseTime current = setFrcResponseTime(result, IDpaTransaction2::FrcResponseTime::k40Ms);
			result.setCurrentResponseTime(current);
			// Get response time
			IDpaTransaction2::FrcResponseTime recommended = getResponseTime(result);
			result.setRecommendedResponseTime(recommended);
			// Set original FRC response time
			m_dpaService->setFrcResponseTime(result.getCurrentResponseTime());
			setFrcResponseTime(result, result.getCurrentResponseTime());
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

	void FrcResponseTime::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "FrcResponseTime instance activate" << std::endl
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

	void FrcResponseTime::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		(void)props;
		TRC_FUNCTION_LEAVE("");
	}

	void FrcResponseTime::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "FrcResponseTime instance deactivate" << std::endl
			<< "******************************"
		);
		m_splitterService->unregisterFilteredMsgHandler(m_mTypes);
		TRC_FUNCTION_LEAVE("");
	}

	///// Interface management

	void FrcResponseTime::attachInterface(IIqrfDpaService *iface) {
		m_dpaService = iface;
	}

	void FrcResponseTime::detachInterface(IIqrfDpaService *iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void FrcResponseTime::attachInterface(IMessagingSplitterService *iface) {
		m_splitterService = iface;
	}

	void FrcResponseTime::detachInterface(IMessagingSplitterService *iface) {
		if (m_splitterService == iface) {
			m_splitterService = nullptr;
		}
	}

	void FrcResponseTime::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void FrcResponseTime::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}
}
