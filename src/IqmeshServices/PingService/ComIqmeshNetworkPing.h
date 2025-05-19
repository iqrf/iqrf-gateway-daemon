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
#include "Trace.h"
#include <list>
#include "JsonUtils.h"

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {
	/// Ping input paramaters struct
	typedef struct {
		uint16_t hwpId = HWPID_DoNotCheck;
		int repeat = 1;
	} TPingInputParams;

	class ComIqmeshNetworkPing : public ComBase {
	public:
		/**
		 * Base constructor
		 */
		ComIqmeshNetworkPing() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		explicit ComIqmeshNetworkPing(Document &doc) : ComBase(doc) {
			parse(doc);
		}

		/**
		 * Destructor
		 */
		virtual ~ComIqmeshNetworkPing(){}

		/**
		 * Returns ping request parameters
		 * @return Ping request parameters
		 */
		const TPingInputParams getPingInputParams() const {
			return m_PingParams;
		}
	protected:
		/**
		 * Populates response document
		 * @param doc Response document
		 * @param res Transaction result
		 */
		void createResponsePayload(Document &doc, const IDpaTransactionResult2 &res) override {
			Pointer("/data/rsp/response").Set(doc, HexStringConversion::encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
		}
	private:
		/**
		 * Parses request document and stores ping request parameters
		 * @param doc Request document
		 */
		void parse(Document &doc) {
			Value *jsonVal;
			// Repeat
			if ((jsonVal = Pointer("/data/repeat").Get(doc))) {
				m_PingParams.repeat = jsonVal->GetInt();
			}
			// HWPID
			if ((jsonVal = Pointer("/data/req/hwpId").Get(doc))) {
				m_PingParams.hwpId = (uint16_t)jsonVal->GetUint();
			}
		}

		/// ping request parameters
		TPingInputParams m_PingParams;
	};
}
