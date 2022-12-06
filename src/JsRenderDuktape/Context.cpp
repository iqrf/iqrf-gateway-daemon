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

	bool Context::findFunction(const std::string &name) {
		bool retval = false;
		if (!m_initialized || name.empty()) {
			duk_pop_n(m_ctx, m_relativeStack);
			throw std::logic_error("JS engine not initialized");
		}
		std::string buffer = name;
		std::replace(buffer.begin(), buffer.end(), '.', ' ');
		std::istringstream iss(buffer);

		std::vector<std::string> items;
		while (true) {
			std::string item;
			if (!(iss >> item) && iss.eof()) {
				break;
			}
			items.push_back(item);
		}

		retval = true;
		m_relativeStack = 0;
		for (const auto &item : items) {
			++m_relativeStack;
			bool result = duk_get_prop_string(m_ctx, -1, item.c_str());
			if (!result) {
				duk_pop_n(m_ctx, m_relativeStack);
				throw std::logic_error("Not found: " + item);
			}
		}

		return retval;
	}

	void Context::callFunction(const std::string &name, const std::string &params, std::string &ret) {
		if (!findFunction(name)) {
			duk_pop_n(m_ctx, m_relativeStack);
			throw std::logic_error("Cannot find driver function: " + name);
		}

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
