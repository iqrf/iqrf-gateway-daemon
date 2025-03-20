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
#include "Trace.h"
#include <list>
#include "JsonUtils.h"

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {
	/// Restart input paramaters
	typedef struct {
		uint16_t hwpId = HWPID_DoNotCheck;
		int repeat = 1;
	} TRestartInputParams;

	class ComIqmeshNetworkRestart : public ComBase {
	public:
		/**
		 * Base constructor
		 */
		ComIqmeshNetworkRestart() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		explicit ComIqmeshNetworkRestart(Document &doc) : ComBase(doc) {
			parse(doc);
		}

		/**
		 * Destructor
		 */
		virtual ~ComIqmeshNetworkRestart() {}

		/**
		 * Returns restart request parameters
		 * @return Restart request parameters
		 */
		const TRestartInputParams getRestartInputParams() const {
			return m_RestartParams;
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
		 * Parses request document and stores restart request parameters
		 * @param doc Request document
		 */
		void parse(Document &doc) {
			Value *jsonVal;
			// Repeat
			if ((jsonVal = Pointer("/data/repeat").Get(doc))) {
				m_RestartParams.repeat = jsonVal->GetInt();
			}
			// HWPID
			if ((jsonVal = Pointer("/data/req/hwpId").Get(doc))) {
				m_RestartParams.hwpId = (uint16_t)jsonVal->GetUint();
			}
		}

		/// restart request parameters
		TRestartInputParams m_RestartParams;
	};
}
