/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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
