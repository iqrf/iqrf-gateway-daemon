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

#include "Context.h"
#include "IJsRenderService.h"
#include "ITraceService.h"
#include "ShapeProperties.h"
#include "Trace.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include <algorithm>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>

/// iqrf namespace
namespace iqrf {
	/**
	 * JsRenderDuktape class
	 */
	class JsRenderDuktape : public IJsRenderService {
	public:
		/**
		 * Constructor
		 */
		JsRenderDuktape();

		/**
		 * Destructor
		 */
		virtual ~JsRenderDuktape();

		/**
		 * Initializes component
		 * @param props Component properties
		 */
		void activate(const shape::Properties *props = 0);

		/**
		 * Modifies component properties
		 * @param props Component properties
		 */
		void modify(const shape::Properties *props);

		/**
		 * Deactivates component
		 */
		void deactivate();

		/**
		 * Creates a new context and loads code, or loads code for existing context
		 * @param contextId Context ID
		 * @param js Code to load
		 * @param driverIdSet Context drivers
		 * @return true if context code was successfully loaded, false otherwise
		 */
		bool loadContextCode(int contextId, const std::string &js, const std::set<int> &driverIdSet) override;

		/**
		 * Assigns context ID for device address
		 * @param address Device address
		 * @param contextId Context ID
		 */
		void mapAddressToContext(int address, int contextId) override;

		/**
		 * Attempts to find suitable context and call function
		 * @param address Address
		 * @param hwpid HW profile ID
		 * @param fname Function name
		 * @param params Function call parameters
		 * @param ret Return value
		 */
		void callContext(int address, int hwpid, const std::string &fname, const std::string &params, std::string &ret) override;

		/**
		 * Returns context driver IDs
		 * @param contextId Context ID
		 * @return Set of context driver IDs
		 */
		std::set<int> getDriverIdSet(int contextId) const override;

		/**
		 * Clears all driver contexts, device and address mapping
		 */
		void clearContexts() override;

		/**
		 * Attaches tracing service interface
		 * @param iface Tracing service interface
		 */
		void attachInterface(shape::ITraceService *iface);

		/**
		 * Detaches tracing service interface
		 * @param iface Tracing service interface
		 */
		void detachInterface(shape::ITraceService *iface);
	private:
		/**
		 * Attempts to find context by device address
		 * @param address Device address
		 * @return Context
		 */
		std::shared_ptr<Context> findAddressContext(int address);

		/**
		 * Attempts to find context by HWPID with default HWPID fallback
		 * @param hwpid HWPID
		 * @return Context
		 */
		std::shared_ptr<Context> findHwpidContext(int hwpid);

		/// context mutex
		mutable std::mutex m_contextMtx;
		/// map of contexts
		std::map<int, std::shared_ptr<Context>> m_contexts;
		/// map of addresses and corresponding context IDs
		std::map<int, int> m_addressContextMap;
		/// map of context IDs and corresponding driver IDs
		std::map<int, std::set<int>> m_contextDriverMap;
	};
}
