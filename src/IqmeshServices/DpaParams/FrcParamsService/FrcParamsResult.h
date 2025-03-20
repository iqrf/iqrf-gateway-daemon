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
	/// DPA frc params result class
	class FrcParamsResult : public ServiceResultBase {
	public:
		/**
		 * Stores action
		 * @param action Action
		 */
		void setAction(TDpaParamAction action) {
			m_action = action;
		}

		/**
		 * Sets FRC response time
		 * @param responseTime FRC response time
		 */
		void setResponseTime(IDpaTransaction2::FrcResponseTime responseTime) {
			m_responseTime = responseTime;
		}

		/**
		 * Sets offline frc
		 * @param offlineFrc Offline frc
		 */
		void setOfflineFrc(bool offlineFrc) {
			m_offlineFrc = offlineFrc;
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
				Pointer("/data/rsp/responseTime").Set(response, m_responseTime);
				Pointer("/data/rsp/offlineFrc").Set(response, m_offlineFrc);
			}

			// Transactions and error codes
			ServiceResultBase::createResponse(response);
		}
	private:
		/// DPA param action
		TDpaParamAction m_action;
		/// FRC response time
		IDpaTransaction2::FrcResponseTime m_responseTime;
		/// Offline FRC
		bool m_offlineFrc;
	};
}
