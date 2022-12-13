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

#include "MngBaseMsg.h"
#include "ISchedulerService.h"

namespace iqrf {

	/**
	 * Scheduler change task state message
	 */
	class SchedulerChangeTaskStateMsg : public MngBaseMsg {
	public:
		/// Delete base constructor
		SchedulerChangeTaskStateMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 * @param schedulerRervice Scheduler service interface
		 */
		SchedulerChangeTaskStateMsg(const Document &doc, ISchedulerService *schedulerService, bool active);

		/**
		 * Destructor
		 */
		virtual ~SchedulerChangeTaskStateMsg() {};

		/**
		 * Handles change scheduler task state request
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
		std::string m_taskId;
		/// Requested task state
		bool m_active;
	};
}
