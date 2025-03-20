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

#include "ApiMsg.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "Trace.h"

#include <iostream>

using namespace rapidjson;

namespace iqrf {

	/**
	 * Base Daemon management class
	 */
	class MngBaseMsg : public ApiMsg {
	public:
		/// Delete base constructor
		MngBaseMsg() = delete;

		/**
		 * Document constructor
		 * @param doc Request document
		 */
		MngBaseMsg(const Document &doc) : ApiMsg(doc) {};

		/**
		 * Destructor
		 */
		virtual ~MngBaseMsg() {};

		/**
		 * Sets error string
		 * @param errorStr Error string
		 */
		void setErrorString(const std::string &errorStr);

		/**
		 * Populates response document
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override;

		/**
		 * Message handler
		 */
		virtual void handleMsg() = 0;
	private:
		/// Error string
		std::string m_errorStr;
	};
}
