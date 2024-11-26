/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

#include "MngSetModeMsg.h"

namespace iqrf {

	MngSetModeMsg::MngSetModeMsg(const Document &doc, IUdpConnectorService *udpConnectorService) : MngBaseMsg(doc) {
		m_udpConnectorService = udpConnectorService;
		std::string mode = Pointer("/data/req/operMode").Get(doc)->GetString();
		m_mode = ModeStringConvertor::str2enum(mode);
	}

	void MngSetModeMsg::handleMsg() {
		if (m_udpConnectorService) {
			m_udpConnectorService->setMode(m_mode);
		} else {
			throw std::logic_error("UdpConnectorService not active.");
		}
	}

	void MngSetModeMsg::createResponsePayload(Document &doc) {
		MngBaseMsg::createResponsePayload(doc);
	}
}
