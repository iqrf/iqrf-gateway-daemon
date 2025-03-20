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
			Pointer("/data/rsp/response").Set(doc, HexStringConversion::encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
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
