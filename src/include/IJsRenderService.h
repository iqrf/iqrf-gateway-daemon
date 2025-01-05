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
		virtual bool loadContextCode(int contextId, const std::string &js, const std::set<int> &driverIdSet) = 0;

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
		virtual std::set<int> getDriverIdSet(int contextId) const = 0;

		/**
		 * Clears all driver contexts, device and address mapping
		 */
		virtual void clearContexts() = 0;
	};
}
