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

#include "SchedulerEditTaskMsg.h"

namespace iqrf {

	SchedulerEditTaskMsg::SchedulerEditTaskMsg(const Document &doc, ISchedulerService *schedulerService) : MngBaseMsg(doc) {
		m_schedulerService = schedulerService;
		m_clientId = Pointer("/data/req/clientId").Get(doc)->GetString();
        m_taskId = Pointer("/data/req/taskId").Get(doc)->GetString();

		const Value* val = Pointer("/data/req/newTaskId").Get(doc);
		m_newTaskId = val ? val->GetString() : m_taskId;
		val = Pointer("/data/req/description").Get(doc);
		if (val) {
			m_description = val->GetString();
		}
		val = Pointer("/data/req/persist").Get(doc);
		if (val) {
			m_persist = val->GetBool();
		}
		val = Pointer("/data/req/enabled").Get(doc);
		if (val) {
			m_enabled = val->GetBool();
		}

		val = Pointer("/data/req/task").Get(doc);
		m_task->CopyFrom(*val, m_task->GetAllocator());
		val = Pointer("/data/req/timeSpec").Get(doc);
		m_timeSpec->CopyFrom(*val, m_timeSpec->GetAllocator());
	}

	void SchedulerEditTaskMsg::handleMsg() {
		try {
			m_taskId = m_schedulerService->editTask(
				m_clientId,
				m_taskId,
				m_newTaskId,
				m_description,
				*m_task,
				*m_timeSpec,
				m_persist,
				m_enabled
			);
		} catch (std::exception &e) {
			std::ostringstream os;
			os << "Cannot edit task: "  << e.what();
			throw std::logic_error(os.str());
		}
	}

	void SchedulerEditTaskMsg::createResponsePayload(Document &doc) {
		Pointer("/data/rsp/clientId").Set(doc, m_clientId);
		Pointer("/data/rsp/taskId").Set(doc, m_taskId);
		MngBaseMsg::createResponsePayload(doc);
	}
}
