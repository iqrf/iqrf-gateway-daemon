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
		m_splitterService->registerFilteredMsgHandler(m_messageTypes, [&](const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document request) {
			handleMsg(messagingId, msgType, std::move(request));
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
		TRC_FUNCTION_LEAVE("");
	}

	///// Message handlers

	void JsonDbApi::handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document request) {
		rapidjson::Document response;
		std::unique_ptr<BaseMsg> msg;

		if (msgType.m_type == "iqrfDb_Enumerate") {
			std::unique_lock<std::mutex> lock(m_enumerateMutex);
			if (m_enumerateMsg) {
				THROW_EXC_TRC_WAR(std::logic_error, "Enumeration already in progress.");
			}
			m_enumerateMsg = std::make_unique<EnumerateMsg>(EnumerateMsg(request));
			m_enumerateMsg->setMessagingId(messagingId);
			m_enumerateMsg->handleMsg(m_dbService);
			return;
		} else if (msgType.m_type == "iqrfDb_GetBinaryOutputs") {
			msg = std::make_unique<GetBinaryOutputsMsg>(GetBinaryOutputsMsg(request));
		} else if (msgType.m_type == "iqrfDb_GetDalis") {
			msg = std::make_unique<GetDalisMsg>(GetDalisMsg(request));
		} else if (msgType.m_type == "iqrfDb_GetDevices") {
			msg = std::make_unique<GetDevicesMsg>(GetDevicesMsg(request));
		} else if (msgType.m_type == "iqrfDb_GetNetworkTopology") {
			msg = std::make_unique<GetNetworkTopologyMsg>(GetNetworkTopologyMsg(request));
		} else if (msgType.m_type == "iqrfDb_GetLights") {
			msg = std::make_unique<GetLightsMsg>(GetLightsMsg(request));
		} else if (msgType.m_type == "iqrfDb_GetSensors") {
			msg = std::make_unique<GetSensorsMsg>(GetSensorsMsg(request));
		} else if (msgType.m_type == "iqrfDb_Reset") {
			msg = std::make_unique<ResetMsg>(ResetMsg(request));
		} else if (msgType.m_type == "iqrfDb_GetDeviceMetadata") {
			msg = std::make_unique<GetDeviceMetadataMsg>(GetDeviceMetadataMsg(request));
		} else if (msgType.m_type == "iqrfDb_SetDeviceMetadata") {
			msg = std::make_unique<SetDeviceMetadataMsg>(SetDeviceMetadataMsg(request));
		}

		try {
			msg->setMessagingId(messagingId);
			msg->handleMsg(m_dbService);
			msg->setStatus("ok", 0);
			msg->createResponse(response);
			m_splitterService->sendMessage(messagingId, std::move(response));
		} catch (std::exception &e) {
			msg->setStatus(e.what(), -1);
			rapidjson::Document errorResponse;
			msg->createResponse(errorResponse);
			m_splitterService->sendMessage(messagingId, std::move(errorResponse));
		}
	}

	void JsonDbApi::sendEnumerationResponse(IIqrfDb::EnumerationProgress progress) {
		std::unique_lock<std::mutex> lock(m_enumerateMutex);
		if (!m_enumerateMsg) {
			if (progress.getStep() == IIqrfDb::EnumerationProgress::Steps::Finish) {
				sendAsyncEnumerationFinishResponse(progress);
			}
			return;
		}
		rapidjson::Document response;
		m_enumerateMsg->setErrorString(progress.getStepMessage());
		m_enumerateMsg->setStatus("ok", 0);
		if (progress.getStep() == IIqrfDb::EnumerationProgress::Steps::Finish) {
			m_enumerateMsg->setFinished();
		}
		m_enumerateMsg->createResponse(response);
		m_splitterService->sendMessage(m_enumerateMsg->getMessagingId(), std::move(response));
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
		msg.setErrorString(progress.getStepMessage());
		msg.setStatus("ok", 0);
		msg.setFinished();
		msg.createResponse(response);
		m_splitterService->sendMessage("", std::move(response));
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
