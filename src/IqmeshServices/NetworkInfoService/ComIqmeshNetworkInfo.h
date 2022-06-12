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

#include "ComBase.h"

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {

	/// Network info input parameters
	typedef struct {
		bool mids = false;
		bool vrns = false;
		bool zones = false;
		bool parents = false;
		uint8_t repeat = 1;
	} TNetworkInfoParams;

	/// Network info request class
	class ComIqmeshNetworkInfo : public ComBase {
	public:
		/**
		 * Base constructor
		 */
		ComIqmeshNetworkInfo() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		ComIqmeshNetworkInfo(Document &doc) : ComBase(doc) {
			parse(doc);
		}

		/**
		 * Destructor
		 */
		virtual ~ComIqmeshNetworkInfo() {};

		/**
		 * Returns Network info request parameters
		 * @return const TNetworkInfoParams
		 */
		const TNetworkInfoParams getNetworkInfoParams() const {
			return m_requestParams;
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
		 * Parses request document and stores request parameters
		 * @param doc Request document
		 */
		void parse(Document &doc) {
			Value *v = Pointer("/data/req/mids").Get(doc);
			if (v) {
				m_requestParams.mids = v->GetBool();
			}
			v = Pointer("/data/req/vrns").Get(doc);
			if (v) {
				m_requestParams.vrns = v->GetBool();
			}
			v = Pointer("/data/req/zones").Get(doc);
			if (v) {
				m_requestParams.zones = v->GetBool();
			}
			v = Pointer("/data/req/parents").Get(doc);
			if (v) {
				m_requestParams.parents = v->GetBool();
			}
			v = Pointer("/data/repeat").Get(doc);
			if (v) {
				m_requestParams.repeat = (uint8_t)v->GetUint();
			}
		}

		/// Request parameters
		TNetworkInfoParams m_requestParams;
	};
}
