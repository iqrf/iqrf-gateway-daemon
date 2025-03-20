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
