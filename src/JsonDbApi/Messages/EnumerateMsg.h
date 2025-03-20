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
		 * Populates response document with enumeration error
		 * @param doc Response document
		 */
		void createErrorResponsePayload(Document &doc);

		/**
		 * Sets enumeration step code
		 * @param stepCode Step code
		 */
		void setStepCode(const uint8_t &stepCode);

		/**
		 * Sets enumeration step message
		 * @param errorStr Step message
		 */
		void setStepString(const std::string &stepStr);

		/**
		 * Sets finished status of enumeration
		 */
		void setFinished();
	private:
		/// Enumeration parameters
		IIqrfDb::EnumParams parameters;
		/// Step code
		uint8_t stepCode;
		/// Step string
		std::string stepStr;
		/// Enumeration finished
		bool finished = false;
	};
}
