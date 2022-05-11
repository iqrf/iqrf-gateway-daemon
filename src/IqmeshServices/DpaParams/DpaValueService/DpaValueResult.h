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
	/// DPA value result class
	class DpaValueResult : public ServiceResultBase {
	public:
		/**
		 * Stores action
		 * @param action Action
		 */
		void setAction(TDpaParamAction action) {
			m_action = action;
		}

		/**
		 * Stores value type
		 * @param type Value type
		 */
		void setValueType(TDpaValueType type) {
			m_valueType = type;
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
				if (m_action == TDpaParamAction::GET) {
					Pointer("/data/rsp/type").Set(response, m_valueType);
				}
			}

			// Transactions and error codes
			ServiceResultBase::createResponse(response);
		}
	private:
		/// DPA param action
		TDpaParamAction m_action;
		/// DPA value type
		TDpaValueType m_valueType;
	};
}
