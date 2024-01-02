/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

#include "IDpaTransactionResult2.h"
#include "IIqrfChannelService.h"
#include "IIqrfDpaService.h"

/// iqrf namespace
namespace iqrf {

	/// Daemon monitoring service API
	class IMonitorService {
	public:
		/**
		 * Get DPA message queue length
		 * @return int Message queue length
		 */
		virtual int getDpaQueueLen() const = 0;

		/**
		 * Get current IQRF channel state
		 * @return IQRF channel state
		 */
		virtual IIqrfChannelService::State getIqrfChannelState() = 0;

		/**
		 * Get current DPA channel state
		 * @return DPA channel state
		 */
		virtual IIqrfDpaService::DpaState getDpaChannelState() = 0;

		/**
		 * Wake up sleeping worker and force monitoring notification on demand
		 */
		virtual void invokeWorker() = 0;

		/**
		 * Destructor
		 */
		virtual ~IMonitorService() {}
	};
}
