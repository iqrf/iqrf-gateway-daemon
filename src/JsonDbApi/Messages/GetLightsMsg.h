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
