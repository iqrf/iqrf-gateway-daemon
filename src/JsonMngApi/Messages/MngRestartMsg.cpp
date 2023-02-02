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

#include "MngRestartMsg.h"

namespace iqrf {

	MngRestartMsg::MngRestartMsg(const Document &doc, ISchedulerService *schedulerService) : MngBaseMsg(doc) {
		m_schedulerService = schedulerService;
		m_timeToExit = Pointer("/data/req/timeToExit").Get(doc)->GetDouble();
	}

	void MngRestartMsg::handleMsg() {
		Document doc;
		Pointer("/task/restart").Set(doc, true);

		std::stringstream ss;
		ss << "Exit scheduled in: " << m_timeToExit << " ms." << std::endl;

		shape::Tracer::get().writeMsg(
			(int)shape::TraceLevel::Information,
			0,
			TRC_MNAME,
			__FILE__,
			__LINE__,
			__FUNCTION__,
			ss.str()
		);

		std::cout << ss.str();

		m_schedulerService->scheduleInternalTask(
			"JsonMngApi",
			"00000000-0000-0000-0000-000000000000",
			doc,
			std::chrono::system_clock::now() + std::chrono::milliseconds((unsigned)m_timeToExit),
			false,
			true
		);
	}

	void MngRestartMsg::createResponsePayload(Document &doc) {
		Pointer("/data/rsp/timeToExit").Set(doc, m_timeToExit);
		MngBaseMsg::createResponsePayload(doc);
	}
}
