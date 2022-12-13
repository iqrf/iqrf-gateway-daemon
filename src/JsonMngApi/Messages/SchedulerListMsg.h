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
	 * Scheduler list tasks message
	 */
	class SchedulerListMsg : public MngBaseMsg {
	public:
		/// Delete base constructor
		SchedulerListMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 * @param schedulerRervice Scheduler service interface
		 */
		SchedulerListMsg(const Document &doc, ISchedulerService *schedulerService);

		/**
		 * Destructor
		 */
		virtual ~SchedulerListMsg();

		/**
		 * Handles list scheduler tasks request
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
		/// Retrieve full tasks
		bool m_details = false;
		/// Scheduler task IDs
		std::vector<ISchedulerService::TaskHandle> m_tasksIds;
		/// Scheduler tasks
		std::vector<rapidjson::Value *> m_tasks;
		/// Tasks doc
		Document *m_tasksDoc = nullptr;
	};
}
