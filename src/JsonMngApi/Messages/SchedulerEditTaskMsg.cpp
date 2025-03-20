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
