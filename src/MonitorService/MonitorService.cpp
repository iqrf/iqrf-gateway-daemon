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
#include "MonitorService.h"

#include "iqrf__MonitorService.hxx"

TRC_INIT_MODULE(iqrf::MonitorService)

namespace iqrf {

	MonitorService::MonitorService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	MonitorService::~MonitorService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// API implementation

	int MonitorService::getDpaQueueLen() const {
		return m_dpaService->getDpaQueueLen();
	}

	IIqrfChannelService::State MonitorService::getIqrfChannelState() {
		return m_dpaService->getIqrfChannelState();
	}

	IIqrfDpaService::DpaState MonitorService::getDpaChannelState() {
		return m_dpaService->getDpaChannelState();
	}

	void MonitorService::invokeWorker() {
		std::unique_lock<std::mutex> workerInvokeLock(m_invokeMutex);
		m_cv.notify_all();
	}

	///// Component lifecycle

	void MonitorService::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************************" << std::endl <<
			"MonitorService instance activate" << std::endl <<
			"******************************************"
		);

		modify(props);

		m_runThread = true;
		m_workerThread = std::thread([&]() {
			worker();
		});

		TRC_FUNCTION_LEAVE("");
	}

	void MonitorService::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");

		using namespace rapidjson;

		const Document& doc = props->getAsJson();
		{
			const Value* v = Pointer("/reportPeriod").Get(doc);
			if (v && v->IsInt()) {
				m_reportPeriod = v->GetInt();
			}
			m_instanceId = Pointer("/instance").Get(doc)->GetString();
		}

		TRC_FUNCTION_LEAVE("");
	}

	void MonitorService::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"**************************************" << std::endl <<
			"MonitorService instance deactivate" << std::endl <<
			"**************************************"
		);

		m_runThread = false;
		m_cv.notify_all();
		if (m_workerThread.joinable()) {
			m_workerThread.join();
		}

		TRC_FUNCTION_LEAVE("");
	}

	///// Private methods

	void MonitorService::handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc) {
		TRC_FUNCTION_ENTER("");

		invokeWorker();

		std::string msgId = rapidjson::Pointer("/data/msgId").Get(doc)->GetString();
		bool verbose = false;
		rapidjson::Value *val = rapidjson::Pointer("/data/returnVerbose").Get(doc);
		if (val && val->IsBool()) {
			verbose = val->GetBool();
		}
		rapidjson::Document rspDoc;
		rapidjson::Pointer("/mType").Set(rspDoc, msgType.m_type);
		rapidjson::Pointer("/data/msgId").Set(rspDoc, msgId);
		rapidjson::Pointer("/data/status").Set(rspDoc, 0);
		if (verbose) {
			rapidjson::Pointer("/data/statusStr").Set(rspDoc, "ok");
		}

		m_splitterService->sendMessage(messagingId, std::move(rspDoc));

		TRC_FUNCTION_LEAVE("");
	}

	rapidjson::Document MonitorService::createMonitorMessage() {
		TRC_FUNCTION_ENTER("");

		static unsigned num = 0;
		int dpaQueueLen = -1;
		int msgQueueLen = -1;
		IIqrfChannelService::State iqrfChannelState = IIqrfChannelService::State::NotReady;
		IIqrfDpaService::DpaState dpaChannelState = IIqrfDpaService::DpaState::NotReady;
		IUdpConnectorService::Mode operMode = IUdpConnectorService::Mode::Unknown;

		using namespace rapidjson;

		if (m_dpaService) {
			dpaQueueLen = m_dpaService->getDpaQueueLen();
			iqrfChannelState = m_dpaService->getIqrfChannelState();
			dpaChannelState = m_dpaService->getDpaChannelState();
		}

		if (m_splitterService) {
			msgQueueLen = m_splitterService->getMsgQueueLen();
		}

		if (m_udpConnectorService) {
			operMode = m_udpConnectorService->getMode();
		}

		auto ts = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		Document doc;
		Pointer("/mType").Set(doc, "ntfDaemon_Monitor");
		Pointer("/data/num").Set(doc, num++);
		Pointer("/data/timestamp").Set(doc, ts);
		Pointer("/data/dpaQueueLen").Set(doc, dpaQueueLen);
		Pointer("/data/iqrfChannelState").Set(doc, IIqrfChannelService::StateStringConvertor::enum2str(iqrfChannelState));
		Pointer("/data/dpaChannelState").Set(doc, IIqrfDpaService::DpaStateStringConvertor::enum2str(dpaChannelState));
		Pointer("/data/msgQueueLen").Set(doc, msgQueueLen);
		Pointer("/data/operMode").Set(doc, ModeStringConvertor::enum2str(operMode));
		return doc;
	}

	void MonitorService::worker() {
		TRC_FUNCTION_ENTER("");

		while (m_runThread) {

			std::unique_lock<std::mutex> lck(m_workerMutex);
			m_cv.wait_for(lck, std::chrono::seconds(m_reportPeriod));

			using namespace rapidjson;

			auto doc = createMonitorMessage();

			std::string gwMonitorRecord;
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			doc.Accept(writer);
			gwMonitorRecord = buffer.GetString();

			m_websocketService->sendMessage(gwMonitorRecord, ""); //send to all connected clients
		}

		TRC_FUNCTION_LEAVE("");
	}

	///// Interfaces

	void MonitorService::attachInterface(IIqrfDpaService* iface) {
		m_dpaService = iface;
	}

	void MonitorService::detachInterface(IIqrfDpaService* iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void MonitorService::attachInterface(IMessagingSplitterService* iface) {
		m_splitterService = iface;
		m_splitterService->registerFilteredMsgHandler(
			m_mTypes,
			[&](const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc) {
				handleMsg(messagingId, msgType, std::move(doc));
			}
		);
	}

	void MonitorService::detachInterface(IMessagingSplitterService* iface) {
		if (m_splitterService == iface) {
			m_splitterService->unregisterFilteredMsgHandler(m_mTypes);
			m_splitterService = nullptr;
		}
	}

	void MonitorService::attachInterface(IUdpConnectorService* iface) {
		m_udpConnectorService = iface;
		m_udpConnectorService->registerModeSetCallback(
			m_instanceId, [&]() {invokeWorker();}
		);
	}

	void MonitorService::detachInterface(IUdpConnectorService* iface) {
		if (m_udpConnectorService == iface) {
			m_udpConnectorService->unregisterModeSetCallback(m_instanceId);
			m_udpConnectorService = nullptr;
		}
	}

	void MonitorService::attachInterface(shape::IWebsocketService* iface) {
		m_websocketService = iface;
	}

	void MonitorService::detachInterface(shape::IWebsocketService* iface) {
		if (m_websocketService == iface) {
			m_websocketService = nullptr;
		}
	}

	void MonitorService::attachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void MonitorService::detachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().removeTracerService(iface);
	}
}
