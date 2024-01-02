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

#include "SchedulerChangeTaskStateMsg.h"

namespace iqrf {

	SchedulerChangeTaskStateMsg::SchedulerChangeTaskStateMsg(const Document &doc, ISchedulerService *schedulerService, bool active) : MngBaseMsg(doc) {
		m_schedulerService = schedulerService;
		m_clientId = Pointer("/data/req/clientId").Get(doc)->GetString();
		m_taskId = Pointer("/data/req/taskId").Get(doc)->GetString();
		m_active = active;
	}

	void SchedulerChangeTaskStateMsg::handleMsg() {
		try {
			m_schedulerService->changeTaskState(m_clientId, m_taskId, m_active);
		} catch (const std::logic_error &e) {
			throw std::logic_error("Client or task ID does not exist.");
		}
	}

	void SchedulerChangeTaskStateMsg::createResponsePayload(Document &doc) {
		Pointer("/data/rsp/clientId").Set(doc, m_clientId);
		Pointer("/data/rsp/taskId").Set(doc, m_taskId);
		MngBaseMsg::createResponsePayload(doc);
	}
}
