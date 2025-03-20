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

#include "SchedulerGetTaskMsg.h"

namespace iqrf {

	SchedulerGetTaskMsg::SchedulerGetTaskMsg(const Document &doc, ISchedulerService *schedulerService) : MngBaseMsg(doc) {
		m_schedulerService = schedulerService;
		m_clientId = Pointer("/data/req/clientId").Get(doc)->GetString();
		m_taskId = Pointer("/data/req/taskId").Get(doc)->GetString();
	}

	void SchedulerGetTaskMsg::handleMsg() {
		try {
			m_taskDoc = new Document();
			m_schedulerService->getTaskDocument(m_clientId, m_taskId, *m_taskDoc);
			m_active = m_schedulerService->isTaskActive(m_clientId, m_taskId);
		} catch (const std::logic_error &e) {
			throw std::logic_error(e.what());
		}
	}

	void SchedulerGetTaskMsg::createResponsePayload(Document &doc) {
		Pointer("/data/rsp/clientId").Set(doc, m_clientId);
		Pointer("/data/rsp/taskId").Set(doc, m_taskId);
		if (getStatus() == 0) {
			Pointer("/data/rsp").Set(doc, *m_taskDoc);
			Pointer("/data/rsp/active").Set(doc, m_active);
		}
		MngBaseMsg::createResponsePayload(doc);
	}
}
