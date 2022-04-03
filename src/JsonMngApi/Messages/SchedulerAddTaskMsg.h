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
#pragma once

#include <sstream>

#include "MngBaseMsg.h"
#include "ISchedulerService.h"

namespace iqrf {

	/**
	 * Scheduler add task message
	 */
	class SchedulerAddTaskMsg : public MngBaseMsg {
	public:
		/// Delete base constructor
		SchedulerAddTaskMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 * @param schedulerRervice Scheduler service interface
		 */
		SchedulerAddTaskMsg(const Document &doc, ISchedulerService *schedulerService);

		/**
		 * Destructor
		 */
		virtual ~SchedulerAddTaskMsg() {};

		/**
		 * Handles add scheduler tasks request
		 */
		void handleMsg() override;

		/**
		 * Populates response document
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override;
	private:
		/// Scheduler service interface
		ISchedulerService *m_schedulerService = nullptr;
		/// Scheduler client ID
		std::string m_clientId;
		/// Scheduler task ID
		int m_taskId;
		/// Cron time
		ISchedulerService::CronType m_cron;
		/// Periodic task?
		bool m_periodic = false;
		/// Task period
		unsigned m_period = 0;
		/// Oneshot task?
		bool m_exactTime = false;
		/// Oneshot start time
		std::chrono::system_clock::time_point m_startTime;
		/// Scheduler task
    	Document *m_task = nullptr;
		/// Persist task after shutdown
		bool m_persist = false;
	};
}
