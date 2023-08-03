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