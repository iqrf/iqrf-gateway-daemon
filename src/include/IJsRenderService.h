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

#include "IJsCacheService.h"
#include "Trace.h"
#include <string>
#include <set>

/// iqrf namespace
namespace iqrf {
	/**
	 * JsRenderService interface
	 */
	class IJsRenderService {
	public:
		// used to map provisionalDrivers load per HWPID
		static const int HWPID_DEFAULT_MAPPING = -0x10000;
		static const int HWPID_MAPPING_SPACE = -0x20000;

		/**
		 * Destructor
		 */
		virtual ~IJsRenderService() {}

		/**
		 * Creates a new context and loads code, or loads code for existing context
		 * @param contextId Context ID
		 * @param js Code to load
		 * @param driverIdSet Context drivers
		 * @return true if context code was successfully loaded, false otherwise
		 */
		virtual bool loadContextCode(int contextId, const std::string &js, const std::set<uint32_t> &driverIdSet) = 0;

		/**
		 * Assigns context ID for device address
		 * @param address Device address
		 * @param contextId Context ID
		 */
		virtual void mapAddressToContext(int address, int contextId) = 0;

		/**
		 * Attempts to find suitable context and call function
		 * @param address Address
		 * @param hwpid HW profile ID
		 * @param fname Function name
		 * @param params Function call parameters
		 * @param ret Return value
		 */
		virtual void callContext(int address, int hwpid, const std::string &fname, const std::string &params, std::string &ret) = 0;

		/**
		 * Returns context driver IDs
		 * @param contextId Context ID
		 * @return Set of context driver IDs
		 */
		virtual std::set<uint32_t> getDriverIdSet(int contextId) const = 0;

		/**
		 * Returns loaded product context ID by device address
		 * @param address Device address
		 * @return std::shared_ptr<int> Poaded product context ID
		 */
		virtual std::shared_ptr<int> getDeviceAddrProductId(int address) const = 0;

		/**
		 * Clears all driver contexts, device and address mapping
		 */
		virtual void clearContexts() = 0;
	};
}
