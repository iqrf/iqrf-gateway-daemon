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

#include "duktape.h"
#include "DpaPerExceptions.h"
#include "StringUtils.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace iqrf {
	/**
	 * Driver context
	 */
	class Context {
	public:
		/**
		 * Constructor
		 */
		Context();

		/**
		 * Destructor
		 */
		virtual ~Context();

		/**
		 * Loads code to duktape context heap
		 * @param js JavaScript code to load
		 */
		void loadCode(const std::string &js);

		/**
		 * Call context function
		 * @param name Function name
		 * @param params Function parameters
		 * @param ret Return value to store
		 */
		void callFunction(const std::string &name, const std::string &params, std::string &ret);
	private:
		/**
		 * Attempts to find called function in context heap
		 * @param ns namespace
		 * @param object object to find
		 */
		void findNamespaceObject(const std::string &ns, const std::string &object);

		/// Driver context initialized
		bool m_initialized = false;
		/// Context heap
		duk_context *m_ctx = nullptr;
		/// Duktape stack item
		int m_relativeStack = 0;
	};
}