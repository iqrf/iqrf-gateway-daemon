/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#include "ApiMsg.h"

namespace iqrf {

	/**
	 * Exit request message
	 */
	class MngExitMsg : public ApiMsg {
	public:
		/// Delete base constructor
		MngExitMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 * @param schedulerRervice Scheduler service interface
		 */
		MngExitMsg(const rapidjson::Document &doc);

		/**
		 * Destructor
		 */
		virtual ~MngExitMsg() {};

		/**
		 * Get specified exit time
		 */
		uint32_t getExitTime() const;

		/**
		 * Populates response document
		 * @param doc Response document
		 */
		void createResponsePayload(rapidjson::Document &doc) override;
	private:
		/// Time to scheduled exit
		uint32_t m_timeToExit = 0;
	};
}
