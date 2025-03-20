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
