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

#include "JsonDbApi.h"

#include "iqrf__JsonDbApi.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonDbApi);

namespace iqrf {

	JsonDbApi::JsonDbApi() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	JsonDbApi::~JsonDbApi() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// Component life cycle methods

	void JsonDbApi::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************" << std::endl <<
			"JsonDbApi instance activate" << std::endl <<
			"******************************"
		);
		modify(props);
		m_splitterService->registerFilteredMsgHandler(m_messageTypes, [&](const MessagingInstance &messaging, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document request) {
			handleMsg(messaging, msgType, std::move(request));
		});
		m_dbService->registerEnumerationHandler(m_instance, [&](IIqrfDb::EnumerationProgress progress) {
			sendEnumerationResponse(progress);
		});
		TRC_FUNCTION_LEAVE("");
	}

	void JsonDbApi::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		const Document &doc = props->getAsJson();
		m_instance = Pointer("/instance").Get(doc)->GetString();
		TRC_FUNCTION_LEAVE("");
	}

	void JsonDbApi::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************" << std::endl <<
			"JsonDbApi instance deactivate" << std::endl <<
			"******************************"
		);
		m_splitterService->unregisterFilteredMsgHandler(m_messageTypes);
		m_dbService->unregisterEnumerationHandler(m_instance);
		TRC_FUNCTION_LEAVE("");
	}

	///// Message handlers

	void JsonDbApi::handleMsg(const MessagingInstance &messaging, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document request) {
		rapidjson::Document response;
		std::unique_ptr<BaseMsg> msg;

		if (msgType.m_type == "iqrfDb_Enumerate") {
			std::unique_lock<std::mutex> lock(m_enumerateMutex);
			if (m_enumerateMsg) {
				sendEnumerationErrorResponse(
					messaging,
					IIqrfDb::EnumerationError::Errors::AlreadyRunning,
					std::move(request)
				);
			} else {
				m_enumerateMsg = std::make_unique<EnumerateMsg>(EnumerateMsg(request));
				m_enumerateMsg->setMessaging(messaging);
				m_enumerateMsg->handleMsg(m_dbService);
			}
			return;
		} else if (msgType.m_type == "iqrfDb_GetBinaryOutputs") {
			msg = std::make_unique<GetBinaryOutputsMsg>(GetBinaryOutputsMsg(request));
		} else if (msgType.m_type == "iqrfDb_GetDevice") {
			msg = std::make_unique<GetDeviceMsg>(GetDeviceMsg(request));
		} else if (msgType.m_type == "iqrfDb_GetDevices") {
			msg = std::make_unique<GetDevicesMsg>(GetDevicesMsg(request));
		} else if (msgType.m_type == "iqrfDb_GetNetworkTopology") {
			msg = std::make_unique<GetNetworkTopologyMsg>(GetNetworkTopologyMsg(request));
		} else if (msgType.m_type == "iqrfDb_GetLights") {
			msg = std::make_unique<GetLightsMsg>(GetLightsMsg(request));
		} else if (msgType.m_type == "iqrfDb_GetSensors") {
			msg = std::make_unique<GetSensorsMsg>(GetSensorsMsg(request));
		} else if (msgType.m_type == "iqrfDb_GetDeviceMetadata") {
			msg = std::make_unique<GetDeviceMetadataMsg>(GetDeviceMetadataMsg(request));
		} else if (msgType.m_type == "iqrfDb_SetDeviceMetadata") {
			msg = std::make_unique<SetDeviceMetadataMsg>(SetDeviceMetadataMsg(request));
		} else if (msgType.m_type == "iqrfDb_Reset" || msgType.m_type == "infoDaemon_Reset") { // Legacy API messages
			msg = std::make_unique<ResetMsg>(ResetMsg(request));
		} else if (msgType.m_type == "infoDaemon_GetBinaryOutputs") {
			msg = std::make_unique<LegacyGetBinaryOutputsMsg>(LegacyGetBinaryOutputsMsg(request));
		} else if (msgType.m_type == "infoDaemon_GetLights") {
			msg = std::make_unique<LegacyGetLightsMsg>(LegacyGetLightsMsg(request));
		} else if (msgType.m_type == "infoDaemon_GetMidMetaData") {
			msg = std::make_unique<LegacyGetMidMetaDataMsg>(LegacyGetMidMetaDataMsg(request));
		} else if (msgType.m_type == "infoDaemon_GetNodeMetaData") {
			msg = std::make_unique<LegacyGetNodeMetaDataMsg>(LegacyGetNodeMetaDataMsg(request));
		} else if (msgType.m_type == "infoDaemon_GetNodes") {
			msg = std::make_unique<LegacyGetNodesMsg>(LegacyGetNodesMsg(request));
		} else if (msgType.m_type == "infoDaemon_GetSensors") {
			msg = std::make_unique<LegacyGetSensorsMsg>(LegacyGetSensorsMsg(request));
		} else if (msgType.m_type == "infoDaemon_MidMetaDataAnnotate") {
			msg = std::make_unique<LegacyMidMetaDataAnnotateMsg>(LegacyMidMetaDataAnnotateMsg(request));
		} else if (msgType.m_type == "infoDaemon_SetMidMetaData") {
			msg = std::make_unique<LegacySetMidMetaDataMsg>(LegacySetMidMetaDataMsg(request));
		} else if (msgType.m_type == "infoDaemon_SetNodeMetaData") {
			msg = std::make_unique<LegacySetNodeMetaDataMsg>(LegacySetNodeMetaDataMsg(request));
		}

		try {
			msg->setMessaging(messaging);
			msg->handleMsg(m_dbService);
			msg->setStatus("ok", 0);
			msg->createResponse(response);
			m_splitterService->sendMessage(messaging, std::move(response));
		} catch (std::exception &e) {
			msg->setStatus(e.what(), -1);
			rapidjson::Document errorResponse;
			msg->createResponse(errorResponse);
			m_splitterService->sendMessage(messaging, std::move(errorResponse));
		}
	}

	void JsonDbApi::sendEnumerationErrorResponse(const MessagingInstance &messaging, IIqrfDb::EnumerationError::Errors errCode, rapidjson::Document request) {
		IIqrfDb::EnumerationError error(errCode);
		Document response;
		EnumerateMsg msg(request);
		msg.setStatus(error.getErrorMessage(), error.getError());
		msg.createErrorResponsePayload(response);
		m_splitterService->sendMessage(messaging, std::move(response));
	}

	void JsonDbApi::sendEnumerationResponse(IIqrfDb::EnumerationProgress progress) {
		std::unique_lock<std::mutex> lock(m_enumerateMutex);
		if (!m_enumerateMsg) {
			if (progress.getStep() == IIqrfDb::EnumerationProgress::Steps::Start) {
				sendAsyncEnumerationFinishResponse(progress);
			} else if (progress.getStep() == IIqrfDb::EnumerationProgress::Steps::Finish) {
				sendAsyncEnumerationFinishResponse(progress);
			}
			return;
		}
		auto messaging = m_enumerateMsg->getMessaging();
		if (messaging == nullptr) {
			TRC_WARNING("No valid messaging instance for synchronous enumeration response.");
			return;
		}
		rapidjson::Document response;
		m_enumerateMsg->setStepCode(progress.getStep());
		m_enumerateMsg->setStepString(progress.getStepMessage());
		m_enumerateMsg->setStatus("ok", 0);
		if (progress.getStep() == IIqrfDb::EnumerationProgress::Steps::Finish) {
			m_enumerateMsg->setFinished();
		}
		m_enumerateMsg->createResponse(response);
		m_splitterService->sendMessage(*messaging, std::move(response));
		if (progress.getStep() == IIqrfDb::EnumerationProgress::Steps::Finish) {
			m_enumerateMsg.reset();
		}
	}

	void JsonDbApi::sendAsyncEnumerationFinishResponse(IIqrfDb::EnumerationProgress progress) {
		Document request, response;
		Document::AllocatorType &allocator = request.GetAllocator();
		request.SetObject();
		Pointer("/mType").Set(request, "iqrfDb_Enumerate", allocator);
		Pointer("/data/msgId").Set(request, "iqrfdb_enumerate_async", allocator);
		Pointer("/data/returnVerbose").Set(request, true, allocator);
		Pointer("/data/req/standards").Set(request, false, allocator);
		EnumerateMsg msg(request);
		msg.setStepCode(progress.getStep());
		msg.setStepString(progress.getStepMessage());
		msg.setStatus("ok", 0);
		msg.setFinished();
		msg.createResponse(response);
		m_splitterService->sendMessage(std::list<MessagingInstance>(), std::move(response));
	}

	///// Service interfaces /////

	void JsonDbApi::attachInterface(IIqrfDb *iface) {
		m_dbService = iface;
	}

	void JsonDbApi::detachInterface(IIqrfDb *iface) {
		if (m_dbService == iface) {
			m_dbService = nullptr;
		}
	}

	void JsonDbApi::attachInterface(IMessagingSplitterService *iface) {
		m_splitterService = iface;
	}

	void JsonDbApi::detachInterface(IMessagingSplitterService *iface) {
		if (m_splitterService == iface) {
			m_splitterService = nullptr;
		}
	}

	void JsonDbApi::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void JsonDbApi::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}

}
