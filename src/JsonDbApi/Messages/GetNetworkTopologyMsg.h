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
	 * Get network topology request message
	 */
	class GetNetworkTopologyMsg : public BaseMsg {
	public:
		/// Delete base constructor
		GetNetworkTopologyMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		GetNetworkTopologyMsg(const Document &doc) : BaseMsg(doc) {};

		/**
		 * Destructor
		 */
		virtual ~GetNetworkTopologyMsg() {};

		/**
		 * Handles get network topology request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override;

		/**
		 * Populates response document with network topology response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override;
	private:
		/// Vector of devices with product information
		std::vector<DeviceProductTuple> devices;
	};
}
