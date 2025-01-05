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
