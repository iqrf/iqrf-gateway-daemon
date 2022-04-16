/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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

#include "BaseMsg.h"

namespace iqrf {

	/**
	 * Enumerate request message
	 */
	class EnumerateMsg : public BaseMsg {
	public:
		/**
		 * Base constructor
		 */
		EnumerateMsg();

		/**
		 * Constructor
		 * @param doc Request document
		 */
		EnumerateMsg(const rapidjson::Document &doc);

		/**
		 * Destructor
		 */
		virtual ~EnumerateMsg() {};

		/**
		 * Handle enumerate request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override;

		/**
		 * Populates response document with enumeration progress
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override;

		/**
		 * Sets enumeration error string
		 * @param errorStr Error string
		 */
		void setErrorString(const std::string &errorStr);

		/**
		 * Sets finished status of enumeration
		 */
		void setFinished();
	private:
		/// Enumeration parameters
		IIqrfDb::EnumParams parameters;
		/// Error code
		uint8_t errorCode;
		/// Error string
		std::string errorStr;
		/// Enumeration finished
		bool finished = false;
	};
}
