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

using namespace rapidjson;

namespace iqrf {

	/**
	 * Get sensors request message
	 */
	class GetSensorsMsg : public BaseMsg {
	public:
		/// Delete base constructor
		GetSensorsMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		GetSensorsMsg(const Document &doc) : BaseMsg(doc) {};

		/**
		 * Destructor
		 */
		virtual ~GetSensorsMsg() {};

		/**
		 * Handles get sensors request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override;

		/**
		 * Populates response document with sensors response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override;
	private:
		/// Map of sensor device tuples
		std::map<uint8_t, std::vector<std::tuple<DeviceSensor, Sensor>>> sensors;
	};
}
