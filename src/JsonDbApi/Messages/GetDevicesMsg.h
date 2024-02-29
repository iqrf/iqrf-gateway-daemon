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
using namespace iqrf::db;

namespace iqrf {

	/**
	 * Get devices request message
	 */
	class GetDevicesMsg : public BaseMsg {
	public:
		/// Delete base constructor
		GetDevicesMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		GetDevicesMsg(const Document &doc);

		/**
		 * Destructor
		 */
		virtual ~GetDevicesMsg() {};

		/**
		 * Handles get devices request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override;

		/**
		 * Populates response document with devices response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override;
	private:
		/// Indicates whether response should contain brief information
		bool brief = false;
		/// Include binary outputs in response
		bool includeBinouts = false;
		/// Include sensors in response
		bool includeSensors = false;
		/// Vector of requested device addresses
		std::vector<uint8_t> requestedDevices;
		/// Vector of devices with product information
		std::vector<DeviceTuple> devices;
		/// Map of sensor device tuples
		std::map<uint8_t, std::vector<std::tuple<DeviceSensor, Sensor>>> sensors;
		/// Map of binout devices
		std::map<uint8_t, uint8_t> binouts;
	};
}
