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
