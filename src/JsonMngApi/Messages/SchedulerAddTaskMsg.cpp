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

#include "HexStringCoversion.h"
#include "SchedulerAddTaskMsg.h"

namespace iqrf {

	SchedulerAddTaskMsg::SchedulerAddTaskMsg(const Document &doc, ISchedulerService *schedulerService) : MngBaseMsg(doc) {
		m_schedulerService = schedulerService;
		m_clientId = Pointer("/data/req/clientId").Get(doc)->GetString();

		const Value *cron = Pointer("/data/req/timeSpec/cronTime").Get(doc);
		auto it = cron->Begin();
		for (int i = 0; i < 7; i++) {
			m_cron[i] = it->GetString();
			it++;
		}

		m_periodic = Pointer("/data/req/timeSpec/periodic").Get(doc)->GetBool();
		m_period = Pointer("/data/req/timeSpec/period").Get(doc)->GetUint();
		m_exactTime = Pointer("/data/req/timeSpec/exactTime").Get(doc)->GetBool();

		const Value *time = Pointer("/data/req/timeSpec/startTime").Get(doc);
		m_startTime = parseTimestamp(time->GetString());

		m_task = new Document();

		const Value *task = Pointer("/data/req/task").Get(doc);
		if (task && (task->IsObject() || task->IsArray())) {
			m_task->CopyFrom(*task, m_task->GetAllocator());
		}

		const Value *persist = Pointer("/data/req/persist").Get(doc);
		if (persist) {
			m_persist = persist->GetBool();
		}
	}

	void SchedulerAddTaskMsg::handleMsg() {
		try {
			if (m_periodic) {
				m_taskId = m_schedulerService->scheduleTaskPeriodic(
					m_clientId,
					*m_task,
					std::chrono::seconds(m_period),
					m_startTime,
					m_persist
				);
			} else if (m_exactTime) {
				m_taskId = m_schedulerService->scheduleTaskAt(
					m_clientId,
					*m_task,
					m_startTime,
					m_persist
				);
			} else {
				m_taskId = m_schedulerService->scheduleTask(
					m_clientId,
					*m_task,
					m_cron,
					m_persist
				);
			}
		} catch (std::exception &e) {
			std::ostringstream os;
			os << "Cannot schedule (client ID: " << m_clientId << ", task ID: " << m_taskId << "): "  << e.what();
			throw std::logic_error(os.str());
		}
	}

	void SchedulerAddTaskMsg::createResponsePayload(Document &doc) {
		Pointer("/data/rsp/clientId").Set(doc, m_clientId);
		Pointer("/data/rsp/taskId").Set(doc, m_taskId);
		MngBaseMsg::createResponsePayload(doc);
	}
}
