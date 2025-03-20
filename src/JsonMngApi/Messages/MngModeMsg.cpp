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

#include "MngModeMsg.h"

namespace iqrf {

	MngModeMsg::MngModeMsg(const Document &doc, IUdpConnectorService *udpConnectorService) : MngBaseMsg(doc) {
		m_udpConnectorService = udpConnectorService;
		std::string mode = Pointer("/data/req/operMode").Get(doc)->GetString();
		m_mode = ModeStringConvertor::str2enum(mode);
	}

	void MngModeMsg::handleMsg() {
		if (m_udpConnectorService) {
			if (m_mode != IUdpConnectorService::Mode::Unknown) {
				m_udpConnectorService->setMode(m_mode);
			}
			m_mode = m_udpConnectorService->getMode();
		} else {
			throw std::logic_error("UdpConnectorService not active.");
		}
	}

	void MngModeMsg::createResponsePayload(Document &doc) {
		Pointer("/data/rsp/operMode").Set(doc, ModeStringConvertor::enum2str(m_mode));
		MngBaseMsg::createResponsePayload(doc);
	}
}
