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

#include <map>

#include "BaseMsg.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

typedef std::tuple<bool, std::string> SetMetadataResponse;

namespace iqrf {

	/**
	 * Database store device metadata request message
	 */
	class SetDeviceMetadataMsg : public BaseMsg {
	public:
		/// Delete base constructor
		SetDeviceMetadataMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		SetDeviceMetadataMsg(const Document &doc);

		/**
		 * Destructor
		 */
		virtual ~SetDeviceMetadataMsg() {};

		/**
		 * Handle reset request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override;

		/**
		 * Populates response document with sensors response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override;
	private:
		/// Map of device addresses and metadata
		std::map<uint8_t, std::shared_ptr<std::string>> metadataRequest;
		/// Map of device addresses and operation status
		std::map<uint8_t, SetMetadataResponse> metadataResponse;
	};
}
