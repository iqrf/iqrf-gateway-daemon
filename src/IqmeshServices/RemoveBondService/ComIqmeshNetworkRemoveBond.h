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

#include "ComBase.h"
#include <set>

namespace iqrf {

	/// RemoveBond input paramaters
	typedef struct {
		/// List of device addresses to remove
		std::set<uint8_t> deviceAddrList;
		/// Unbond all devices from network
		bool allNodes = false;
		/// Hardware profile ID filter
		uint16_t hwpId = HWPID_DoNotCheck;
		/// Coordinator only
		bool coordinatorOnly = false;
		/// Number of transaction repeats on failure
		int repeat = 1;
	} TRemoveBondRequestParams;

	class ComIqmeshNetworkRemoveBond : public ComBase {
	public:
		/// Delete default constructor
		ComIqmeshNetworkRemoveBond() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		explicit ComIqmeshNetworkRemoveBond(rapidjson::Document& doc) : ComBase(doc) {
			parse(doc);
		}

		/**
		 * Destructor
		 */
		virtual ~ComIqmeshNetworkRemoveBond() {}

		/**
		 * Get request parameters
		 * @return const TRemoveBondRequestParams
		 */
		const TRemoveBondRequestParams getRequestParameters() const {
			return m_requestParams;
		}

	protected:
		/**
		 * Populate response document with response payload
		 * @param doc Response document
		 * @param res Transaction result
		 */
		void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override {
			rapidjson::Pointer("/data/rsp/response").Set(doc, HexStringConversion::encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
		}

	private:
		/**
		 * Parses request document into class members
		 * @param doc Request document
		 */
		void parse(rapidjson::Document& doc) {
			rapidjson::Value* jsonValue;
			m_requestParams.deviceAddrList.clear();
			// addresses specified
			if ((jsonValue = rapidjson::Pointer("/data/req/deviceAddr").Get(doc))) {
				/// integer
				if (jsonValue->IsInt()) {
					uint8_t addr = static_cast<uint8_t>(jsonValue->GetUint());
					m_requestParams.deviceAddrList.insert(addr);
				}
				/// array of integers
				if (jsonValue->IsArray()) {
					for (auto itr = jsonValue->Begin(); itr != jsonValue->End(); ++itr) {
						if (itr->IsInt()) {
							uint8_t addr = static_cast<uint8_t>(itr->GetUint());
							m_requestParams.deviceAddrList.insert(addr);
						}
					}
				}
			}
			// entire network
			if ((jsonValue = rapidjson::Pointer("/data/req/allNodes").Get(doc))) {
				m_requestParams.allNodes = jsonValue->GetBool();
			}
			// hwpId
			if ((jsonValue = rapidjson::Pointer("/data/req/hwpId").Get(doc))) {
				m_requestParams.hwpId = static_cast<uint16_t>(jsonValue->GetUint());
			}
			// coordinator only
			if ((jsonValue = rapidjson::Pointer("/data/req/coordinatorOnly").Get(doc))) {
				m_requestParams.coordinatorOnly = jsonValue->GetBool();
			}
			// repeat
			if ((jsonValue = rapidjson::Pointer("/data/repeat").Get(doc))) {
				m_requestParams.repeat = jsonValue->GetInt();
			}
		}

		/// Request parameters
		TRemoveBondRequestParams m_requestParams;
	};
}
