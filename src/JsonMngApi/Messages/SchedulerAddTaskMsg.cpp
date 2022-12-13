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

#include "SchedulerAddTaskMsg.h"

#include "rapidjson/pointer.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/schema.h"
#include "rapidjson/writer.h"

namespace iqrf {

	SchedulerAddTaskMsg::SchedulerAddTaskMsg(const Document &doc, ISchedulerService *schedulerService) : MngBaseMsg(doc) {
		m_schedulerService = schedulerService;
		m_clientId = Pointer("/data/req/clientId").Get(doc)->GetString();

		const Value *val = Pointer("/data/req/taskId").Get(doc);
		if (val) {
			m_taskId = val->GetString();
		}
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

	void SchedulerAddTaskMsg::handleMsg() {
		try {
			if (m_schedulerService->getTask(m_clientId, m_taskId)) {
				throw std::logic_error("Task already exists");
			}
			m_taskId = m_schedulerService->addTask(m_clientId, m_taskId, m_description, *m_task, *m_timeSpec, m_persist, m_enabled);
		} catch (const std::exception &e) {
			std::ostringstream os;
			os << "Cannot schedule task: " << e.what();
			throw std::logic_error(os.str());
		}
	}

	void SchedulerAddTaskMsg::createResponsePayload(Document &doc) {
		Pointer("/data/rsp/clientId").Set(doc, m_clientId);
		if (m_taskId != "00000000-0000-0000-0000-000000000000") {
			Pointer("/data/rsp/taskId").Set(doc, m_taskId);
		}
		MngBaseMsg::createResponsePayload(doc);
	}
}
