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

#include "BaseMsg.h"

using namespace rapidjson;

namespace iqrf {

	/**
	 * Get device request message
	 */
	class GetDeviceMsg : public BaseMsg {
	public:
		/// Delete base constructor
		GetDeviceMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		GetDeviceMsg(const Document &doc);

		/**
		 * Destructor
		 */
		virtual ~GetDeviceMsg() {};

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
		// Device address
		uint8_t address;
		/// Vector of devices with product information
		TDevice device;
		/// Product info
		TProduct product;
		/// Sensors
		std::map<uint8_t, Sensor> sensors;
		/// Binout count
		uint8_t binouts = 0;
	};
}
