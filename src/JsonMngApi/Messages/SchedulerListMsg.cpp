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

#include "SchedulerListMsg.h"

namespace iqrf {

	SchedulerListMsg::SchedulerListMsg(const Document &doc, ISchedulerService *schedulerService) : MngBaseMsg(doc) {
		m_schedulerService = schedulerService;
		m_clientId = Pointer("/data/req/clientId").Get(doc)->GetString();
		const Value* val = Pointer("/data/req/details").Get(doc);
		if (val) {
			m_details = val->GetBool();
		}
	}

	SchedulerListMsg::~SchedulerListMsg() {
		if (m_tasksDoc != nullptr) {
			delete m_tasksDoc;
		}
		for (auto task : m_tasks) {
			delete task;
		}
		m_tasks.clear();
	}

	void SchedulerListMsg::handleMsg() {
		if (m_details) {
			m_tasksDoc = new Document();
			m_tasks = m_schedulerService->getTasks(m_clientId, m_tasksDoc->GetAllocator());
		} else {
			m_tasksIds = m_schedulerService->getTaskIds(m_clientId);
		}
	}

	void SchedulerListMsg::createResponsePayload(Document &doc) {
		Pointer("/data/rsp/clientId").Set(doc, m_clientId);
		Value array(kArrayType);
		Document::AllocatorType &allocator = doc.GetAllocator();
		if (m_details) {
			for (auto task : m_tasks) {
				array.PushBack(*task, allocator);
			}
		} else {
			for (auto taskId : m_tasksIds) {
				array.PushBack(Value(taskId.c_str(), allocator), allocator);
			}
		}
		Pointer("/data/rsp/tasks").Set(doc, array);
		MngBaseMsg::createResponsePayload(doc);
	}
}
