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
