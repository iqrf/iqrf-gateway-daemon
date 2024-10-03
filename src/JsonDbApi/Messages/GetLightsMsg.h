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
#include <set>

using namespace rapidjson;

namespace iqrf {

	/**
	 * Get lights request message
	 */
	class GetLightsMsg : public BaseMsg {
	public:
		/// Delete base constructor
		GetLightsMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		GetLightsMsg(const Document &doc) : BaseMsg(doc) {};

		/**
		 * Destructor
		 */
		virtual ~GetLightsMsg() {};

		/**
		 * Handles get lights request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override;

		/**
		 * Populates response document with lights response
		 */
		void createResponsePayload(Document &doc) override;
	private:
		/// Set of device addresses implementing lights standard
		std::set<uint8_t> lights;
	};
}
