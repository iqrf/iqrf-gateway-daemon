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
