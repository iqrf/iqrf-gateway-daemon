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

#define IMessagingSplitterService_EXPORTS

#include "JsonDpaApiRaw.h"
#include "iqrf__JsonDpaApiRaw.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonDpaApiRaw)

using namespace rapidjson;

namespace iqrf {

	JsonDpaApiRaw::JsonDpaApiRaw() {
		TRC_FUNCTION_ENTER("");
		m_objectFactory.registerClass<ComRaw>(m_filters[0]);
		m_objectFactory.registerClass<ComRawHdp>(m_filters[1]);
		TRC_FUNCTION_LEAVE("");
	}

	JsonDpaApiRaw::~JsonDpaApiRaw() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// Component lifecycle methods

	void JsonDpaApiRaw::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************" << std::endl <<
			"JsonDpaApiRaw instance activate" << std::endl <<
			"******************************"
		);

		modify(props);

		m_splitterService->registerFilteredMsgHandler(m_filters,
			[&](const MessagingInstance& messaging, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
		{
			handleMsg(messaging, msgType, std::move(doc));
		});

		if (m_asyncDpaMessage) {
			m_dpaService->registerAsyncMessageHandler(m_instanceName, [&](const DpaMessage& dpaMessage) {
				handleAsyncDpaMessage(dpaMessage);
			});
		}

		TRC_FUNCTION_LEAVE("");
	}

	void JsonDpaApiRaw::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");

		const Document& doc = props->getAsJson();

		const Value* v = Pointer("/instance").Get(doc);
		if (v && v->IsString()) {
			m_instanceName = v->GetString();
		}

		v = Pointer("/asyncDpaMessage").Get(doc);
		if (v && v->IsBool()) {
			m_asyncDpaMessage = v->GetBool();
		}

		TRC_FUNCTION_LEAVE("");
	}

	void JsonDpaApiRaw::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************" << std::endl <<
			"JsonDpaApiRaw instance deactivate" << std::endl <<
			"******************************"
		);

		m_splitterService->unregisterFilteredMsgHandler(m_filters);
		m_dpaService->unregisterAsyncMessageHandler(m_instanceName);

		TRC_FUNCTION_LEAVE("");
	}

	///// Message handling

	void JsonDpaApiRaw::handleMsg(const MessagingInstance& messaging, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc) {
		TRC_FUNCTION_ENTER(
			PAR(messaging.to_string()) <<
			NAME_PAR(mType, msgType.m_type) <<
			NAME_PAR(major, msgType.m_major) <<
			NAME_PAR(minor, msgType.m_minor) <<
			NAME_PAR(micro, msgType.m_micro)
		);

		std::unique_ptr<ComNadr> com = m_objectFactory.createObject(msgType.m_type, doc);

		if (m_dbService && m_dbService->getMetadataToMessages()) {
			Document metaDataDoc;
			try {
				metaDataDoc = m_dbService->getDeviceMetadataDoc(com->getNadr());
			} catch (const std::exception &e) {
				TRC_WARNING(e.what())
			}
			com->setMidMetaData(metaDataDoc);
		}

		auto trn = m_dpaService->executeDpaTransaction(com->getDpaRequest(), com->getTimeout());
		auto res = trn->get();

		Document respDoc;
		com->setStatus(res->getErrorString(), res->getErrorCode());
		com->createResponse(respDoc, *res);

		Pointer("/mType").Set(respDoc, msgType.m_type);

		m_splitterService->sendMessage(messaging, std::move(respDoc));

		TRC_FUNCTION_LEAVE("");
	}

	void JsonDpaApiRaw::handleAsyncDpaMessage(const DpaMessage &msg) {
		TRC_FUNCTION_ENTER("");

		Document fakeRequest;
		Pointer("/mType").Set(fakeRequest, "iqrfRaw");
		Pointer("/data/msgId").Set(fakeRequest, "async");
		std::string rData = HexStringConversion::encodeBinary(msg.DpaPacket().Buffer, msg.GetLength());
		//Pointer("/data/req/rData").Set(fakeRequest, "00.00.00.00.00.00");
		Pointer("/data/req/rData").Set(fakeRequest, rData);
		Document respDoc;

		ComRaw asyncResp(fakeRequest);
		FakeAsyncTransactionResult res(msg);

		asyncResp.setStatus(res.getErrorString(), res.getErrorCode());
		asyncResp.createResponse(respDoc, res);

		//update message type - type is the same for request/response
		Pointer("/mType").Set(respDoc, "iqrfRaw");

		m_splitterService->sendMessage(std::list<MessagingInstance>(), std::move(respDoc));

		TRC_FUNCTION_LEAVE("");
	}

	/// Interface management

	void JsonDpaApiRaw::attachInterface(IIqrfDb *iface) {
		m_dbService = iface;
	}

	void JsonDpaApiRaw::detachInterface(IIqrfDb *iface) {
		if (m_dbService == iface) {
			m_dbService = nullptr;
		}
	}

	void JsonDpaApiRaw::attachInterface(IIqrfDpaService* iface){
		m_dpaService = iface;
	}

	void JsonDpaApiRaw::detachInterface(IIqrfDpaService* iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void JsonDpaApiRaw::attachInterface(IMessagingSplitterService* iface) {
		m_splitterService = iface;
	}

	void JsonDpaApiRaw::detachInterface(IMessagingSplitterService* iface) {
		if (m_splitterService == iface) {
			m_splitterService = nullptr;
		}
	}

	void JsonDpaApiRaw::attachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void JsonDpaApiRaw::detachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().removeTracerService(iface);
	}

}
