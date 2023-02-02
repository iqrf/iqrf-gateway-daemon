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

#include "SchedulerRemoveTaskMsg.h"

namespace iqrf {

	SchedulerRemoveTaskMsg::SchedulerRemoveTaskMsg(const Document &doc, ISchedulerService *schedulerService) : MngBaseMsg(doc) {
		m_schedulerService = schedulerService;
		m_clientId = Pointer("/data/req/clientId").Get(doc)->GetString();
		m_taskId = Pointer("/data/req/taskId").Get(doc)->GetString();
	}

	void SchedulerRemoveTaskMsg::handleMsg() {
		const Value *task = m_schedulerService->getTask(m_clientId, m_taskId);
		if (task) {
			m_schedulerService->removeTask(m_clientId, m_taskId);
		} else {
			throw std::logic_error("Cannot remove task: task does not exist.");
		}
	}

	void SchedulerRemoveTaskMsg::createResponsePayload(Document& doc) {
		Pointer("/data/rsp/clientId").Set(doc, m_clientId);
		Pointer("/data/rsp/taskId").Set(doc, m_taskId);
		MngBaseMsg::createResponsePayload(doc);
	}
}
