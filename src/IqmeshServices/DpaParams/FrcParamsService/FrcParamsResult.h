/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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
