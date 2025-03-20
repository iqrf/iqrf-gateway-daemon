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
