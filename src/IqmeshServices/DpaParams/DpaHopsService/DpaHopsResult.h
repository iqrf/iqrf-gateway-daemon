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

#include "IDpaParamsService.h"
#include "IDpaTransaction2.h"
#include "IDpaTransactionResult2.h"
#include "IMessagingSplitterService.h"
#include "rapidjson/document.h"
#include "ServiceResultBase.h"

#include <list>
#include <memory>
#include <set>

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {
	/// DPA hops result class
	class DpaHopsResult : public ServiceResultBase {
	public:
		/**
		 * Stores action
		 * @param action Action
		 */
		void setAction(TDpaParamAction action) {
			m_action = action;
		}

		/**
		 * Sets request hops
		 * @param requestHops Request hops
		 */
		void setRequestHops(const uint8_t &requestHops) {
			m_requestHops = requestHops;
		}

		/**
		 * Sets response hops
		 * @param requestHops Response hops
		 */
		void setResponseHops(const uint8_t &responseHops) {
			m_responseHops = responseHops;
		}

		/**
		 * Populates response document
		 * @param response response document
		 */
		void createResponse(Document &response) {
			// Default parameters
			ServiceResultBase::setResponseMetadata(response);

			// Service results
			if (m_status == 0) {
				Pointer("/data/rsp/action").Set(response, dpaParamActionEnumStringMap[m_action]);
				Pointer("/data/rsp/requestHops").Set(response, m_requestHops);
				Pointer("/data/rsp/responseHops").Set(response, m_responseHops);
			}

			// Transactions and error codes
			ServiceResultBase::createResponse(response);
		}
	private:
		/// DPA param action
		TDpaParamAction m_action;
		/// Request hops
		uint8_t m_requestHops;
		/// Response hops
		uint8_t m_responseHops;
	};
}
