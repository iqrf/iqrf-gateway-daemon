/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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
#include "IDpaParamsService.h"
#include "Trace.h"

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {
	/// DpaValue input parameters struct
	typedef struct {
		TDpaParamAction action = TDpaParamAction::GET;
		TDpaValueType type = TDpaValueType::DpaValueType_RSSI;
		uint8_t repeat = 1;
	} TDPaValueInputParams;

	/// DPA value service com class
	class ComIqmeshNetworkDpaValue : public ComBase {
	public:
		/**
		 * Base constructor
		 */
		ComIqmeshNetworkDpaValue() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		explicit ComIqmeshNetworkDpaValue(Document &doc) : ComBase(doc) {
			parse(doc);
		}

		/**
		 * Destructor
		 */
		virtual ~ComIqmeshNetworkDpaValue() {};

		/**
		 * Returns dpa value request parameters
		 * @return Request parameters
		 */
		const TDPaValueInputParams getDpaValueParams() const {
			return m_params;
		}
	protected:
		/**
		 * Populates response document
		 * @param doc Response document
		 * @param res Transaction result
		 */
		void createResponsePayload(Document &doc, const IDpaTransactionResult2 &res) override {
			Pointer("/data/rsp/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
		}
	private:
		/**
		 * Parses request document and stores DpaValue request parameters
		 * @param doc Request document
		 */
		void parse(Document &doc) {
			// Action
			Value *v = Pointer("/data/req/action").Get(doc);
			if (v) {
				m_params.action = dpaParamActionStringEnumMap[v->GetString()];
			}
			// Value type
			v = Pointer("/data/req/type").Get(doc);
			if (v) {
				m_params.type = (TDpaValueType)v->GetUint();
			}
			// Transaction repeats
			v = Pointer("/data/repeat").Get(doc);
			if (v) {
				m_params.repeat = (uint8_t)v->GetUint();
			}
		}

		/// DpaValue input parameters
		TDPaValueInputParams m_params;
	};
}
