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
#include "Trace.h"

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {
	/// FRC command
	enum FrcCommand {
		IQRF_2BITS_FRC = 0x10,
		IQRF_1BYTE_FRC = 0x90,
		IQRF_2BYTE_FRC = 0xE0,
		IQRF_4BYTE_FRC = 0xF9,
		USER_2BITS_FRC = 0x40,
		USER_1BYTE_FRC = 0xC0,
		USER_2BYTE_FRC = 0xF0,
		USER_4BYTE_FRC = 0xFC
	};

	/// FRC Response Time input parameters
	typedef struct {
		uint8_t command = 0;
		uint8_t repeat = 1;
	} TFrcResponseTimeParams;

	/// FRC Response Time request params class
	class ComIqmeshMaintenanceFrcResponse : public ComBase {
	public:
		/**
		 * Base constructor
		 */
		ComIqmeshMaintenanceFrcResponse() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		explicit ComIqmeshMaintenanceFrcResponse(Document &doc) : ComBase(doc) {
			parse(doc);
		}

		/**
		 * Destructor
		 */
		virtual ~ComIqmeshMaintenanceFrcResponse() {};

		/**
		 * Returns frc response time request parameters
		 * @return const TFrcResponseTimeParams
		 */
		const TFrcResponseTimeParams getFrcResponseTimeParams() const {
			return m_requestParams;
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
		 * Parses request document and stores FRC response time request parameters
		 * @param doc Request document
		 */
		void parse(Document &doc) {
			// FRC command
			m_requestParams.command = (uint8_t)Pointer("/data/req/command").Get(doc)->GetUint();
			// Transaction repeats
			Value *v = Pointer("/data/repeat").Get(doc);
			if (v) {
				m_requestParams.repeat = (uint8_t)v->GetUint();
			}
		}
		/// Request parameters
		TFrcResponseTimeParams m_requestParams;
	};
}
