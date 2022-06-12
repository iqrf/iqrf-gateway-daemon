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

#include "NetworkInfoService.h"
#include "iqrf__NetworkInfoService.hxx"

TRC_INIT_MODULE(iqrf::NetworkInfoService);

/// iqrf namespace
namespace iqrf {

	NetworkInfoService::NetworkInfoService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	NetworkInfoService::~NetworkInfoService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// Component lifecycle

	void NetworkInfoService::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "NetworkInfoService instance activate" << std::endl
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

	void NetworkInfoService::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		(void)props;
		TRC_FUNCTION_LEAVE("");
	}

	void NetworkInfoService::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "NetworkInfoService instance deactivate" << std::endl
			<< "******************************"
		);
		m_splitterService->unregisterFilteredMsgHandler(m_mTypes);
		TRC_FUNCTION_LEAVE("");
	}

	///// Interfaces

	void NetworkInfoService::attachInterface(IIqrfNetworkInfo *iface) {
		m_networkInfo = iface;
	}

	void NetworkInfoService::detachInterface(IIqrfNetworkInfo *iface) {
		if (m_networkInfo == iface) {
			m_networkInfo = nullptr;
		}
	}

	void NetworkInfoService::attachInterface(IMessagingSplitterService *iface) {
		m_splitterService = iface;
	}

	void NetworkInfoService::detachInterface(IMessagingSplitterService *iface) {
		if (m_splitterService == iface) {
			m_splitterService = nullptr;
		}
	}

	void NetworkInfoService::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void NetworkInfoService::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}

	///// Message handler

	void NetworkInfoService::handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, Document doc) {
		TRC_FUNCTION_ENTER(PAR(messagingId) <<
			NAME_PAR(mType, msgType.m_type) <<
			NAME_PAR(major, msgType.m_major) <<
			NAME_PAR(minor, msgType.m_minor) <<
			NAME_PAR(patch, msgType.m_micro)
		);

		ComIqmeshNetworkInfo params(doc);
		m_requestParams = params.getNetworkInfoParams();
		NetworkInfoResult result;
		result.setMessageType(msgType.m_type);
		result.setMessageId(params.getMsgId());
		result.setVerbose(params.getVerbose());
		result.setRetrieveMids(m_requestParams.mids);
		result.setRetrieveVrns(m_requestParams.vrns);
		result.setRetrieveZones(m_requestParams.zones);
		result.setRetrieveParents(m_requestParams.parents);

		Document response;

		try {
			m_networkInfo->getNetworkInfo(result, m_requestParams.repeat);
		} catch (const std::exception &e) {
			CATCH_EXC_TRC_WAR(std::exception, e, e.what());
			result.setStatus(m_networkInfo->getErrorCode(), e.what());
		}

		auto transactions = m_networkInfo->getTransactionResults();
		std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator itr;
		for (itr = transactions.begin(); itr != transactions.end(); itr++) {
			result.addTransactionResult(*itr);
		}

		result.createResponse(response);
		m_splitterService->sendMessage(messagingId, std::move(response));

		TRC_FUNCTION_LEAVE("");
	}
}
