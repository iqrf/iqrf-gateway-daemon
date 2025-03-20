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
