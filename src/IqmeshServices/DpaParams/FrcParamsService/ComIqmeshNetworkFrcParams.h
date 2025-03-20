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

#include "ComBase.h"
#include "IDpaParamsService.h"
#include "IDpaTransaction2.h"
#include "Trace.h"

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {
	/// FrcParams input parameters struct
	typedef struct {
		TDpaParamAction action = TDpaParamAction::GET;
		IDpaTransaction2::FrcResponseTime responseTime = IDpaTransaction2::FrcResponseTime::k40Ms;
		bool offlineFrc = false;
		uint8_t repeat = 1;
	} TFrcParamsInputParams;

	/// DPA frc params service com class
	class ComIqmeshNetworkFrcParams : public ComBase {
	public:
		/**
		 * Base constructor
		 */
		ComIqmeshNetworkFrcParams() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		explicit ComIqmeshNetworkFrcParams(Document &doc) : ComBase(doc) {
			parse(doc);
		}

		/**
		 * Destructor
		 */
		virtual ~ComIqmeshNetworkFrcParams() {};

		/**
		 * Returns dpa frc params request parameters
		 * @return Request parameters
		 */
		const TFrcParamsInputParams getFrcParams() const {
			return m_params;
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
		 * Parses request document and stores FrcParams request parameters
		 * @param doc Request document
		 */
		void parse(Document &doc) {
			// Action
			Value *v = Pointer("/data/req/action").Get(doc);
			if (v) {
				m_params.action = dpaParamActionStringEnumMap[v->GetString()];
			}
			// FRC response time
			v = Pointer("/data/req/responseTime").Get(doc);
			if (v) {
				m_params.responseTime = (IDpaTransaction2::FrcResponseTime)v->GetUint();
			}
			// Offline FRC
			v = Pointer("/data/req/offlineFrc").Get(doc);
			if (v) {
				m_params.offlineFrc = v->GetBool();
			}
			// Transaction repeats
			v = Pointer("/data/repeat").Get(doc);
			if (v) {
				m_params.repeat = (uint8_t)v->GetUint();
			}
		}

		/// FrcParams input parameters
		TFrcParamsInputParams m_params;
	};
}
