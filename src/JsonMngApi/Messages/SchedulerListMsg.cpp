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
