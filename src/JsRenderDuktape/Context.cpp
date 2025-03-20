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

#include "Context.h"

namespace iqrf {

	Context::Context() {
		m_ctx = duk_create_heap_default();
		if (!m_ctx) {
			std::cerr << "Failed to create a Duktape heap." << std::endl;
			throw std::logic_error("Failed to create a Duktape heap.\n");
		}
		duk_push_global_object(m_ctx);
	}

	Context::~Context() {
		duk_destroy_heap(m_ctx);
	}

	void Context::loadCode(const std::string &js) {
		duk_push_string(m_ctx, js.c_str());
		if (duk_peval(m_ctx) != 0) {
			std::string errstr = duk_safe_to_string(m_ctx, -1);
			std::cerr << "Error in driver scripts: " << errstr << std::endl;
			throw std::logic_error(errstr);
		}
		duk_pop(m_ctx);
		m_initialized = true;
	}

	void Context::findNamespaceObject(const std::string &ns, const std::string &object) {
		auto tokens = StringUtils::split(ns, ".");
		m_relativeStack = 0;
		for (const auto &item : tokens) {
			++m_relativeStack;
			bool exists = duk_get_prop_string(m_ctx, -1, item.c_str());
			if (!exists) {
				duk_pop_n(m_ctx, m_relativeStack);
				throw PeripheralException("Peripheral " + item + " namespace not found.");
			}
		}
		if (StringUtils::endsWith(object, "_req") || StringUtils::endsWith(object, "_rsp"))  {
			++m_relativeStack;
			std::string fn = object.substr(0, object.length() - 4);
			bool exists = duk_get_prop_string(m_ctx, -1, fn.c_str());
			if (!exists) {
				duk_pop_n(m_ctx, m_relativeStack);
				throw PeripheralCommandException("Peripheral " + ns + " namespace object " + fn + " not found.");
			}
			duk_pop_n(m_ctx, 1);
			m_relativeStack--;
		}
		++m_relativeStack;
		bool exists = duk_get_prop_string(m_ctx, -1, object.c_str());
		if (!exists) {
			duk_pop_n(m_ctx, m_relativeStack);
			throw std::logic_error("Cannot find driver function: " + ns + '.' + object);
		}
	}

	void Context::callFunction(const std::string &name, const std::string &params, std::string &ret) {
		auto pos = name.find_last_of('.');
		if (pos == std::string::npos) {
			throw std::logic_error("Invalid namespace and function format: " + name);
		}
		std::string ns = name.substr(0, pos);
		std::string object = name.substr(pos + 1, name.length() -1);
		findNamespaceObject(ns, object);

		duk_push_string(m_ctx, params.c_str());
		duk_json_decode(m_ctx, -1);

		int result = duk_pcall(m_ctx, 1);
		std::string error;
		if (result != 0) {
			duk_dup(m_ctx, -1);
			error = duk_safe_to_string(m_ctx, -1);
			duk_pop(m_ctx);
		}

		ret = duk_json_encode(m_ctx, -1);
		if (result != 0) {
			duk_pop_n(m_ctx, m_relativeStack);
			throw std::logic_error(error);
		}

		duk_pop_n(m_ctx, m_relativeStack);
	}
}
