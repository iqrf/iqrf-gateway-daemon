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

#include "MngExitMsg.h"

namespace iqrf {

	MngExitMsg::MngExitMsg(const Document &doc, ISchedulerService *schedulerService) : MngBaseMsg(doc) {
		m_schedulerService = schedulerService;
		m_timeToExit = Pointer("/data/req/timeToExit").Get(doc)->GetUint();
	}

	void MngExitMsg::handleMsg() {
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

	void MngExitMsg::createResponsePayload(Document &doc) {
		Pointer("/data/rsp/timeToExit").Set(doc, m_timeToExit);
		MngBaseMsg::createResponsePayload(doc);
	}
}
