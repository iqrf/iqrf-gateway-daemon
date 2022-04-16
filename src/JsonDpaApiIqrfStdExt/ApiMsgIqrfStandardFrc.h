/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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

#include "ApiMsgIqrfStandard.h"
#include "IJsRenderService.h"
#include "JsDriverSensor.h"

using namespace rapidjson;

namespace iqrf {
	class ApiMsgIqrfStandardFrc : public ApiMsgIqrfStandard {
	public:
		/// Delete implicit constructor
		ApiMsgIqrfStandardFrc() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		ApiMsgIqrfStandardFrc(const Document& doc) : ApiMsgIqrfStandard(doc) {
			{
				const Value *val = Pointer("/data/req/param/getExtraResult").Get(doc);
				if (val && val->IsBool()) {
					m_getExtraResult = val->GetBool();
				}
			}
			{
				const Value *val = Pointer("/data/req/param/extFormat").Get(doc);
				if (val && val->IsBool()) {
					m_getExtFormat = val->GetBool();
				}
			}
			{
				const std::string mType = getMType();
				if (mType != "iqrfSensor_Frc") {
					return;
				}
				const Value *val = Pointer("/data/req/param/sensorIndex").Get(doc);
				if (val && val->IsUint()) {
					hasSensorIndex = true;
					sensorIndex = val->GetUint();
				}
			}
			{
				const Value *val = Pointer("/data/req/param/selectedNodes").Get(doc);
				if (val) {
					m_selectedNodes.CopyFrom(*val, m_selectedNodes.GetAllocator());
				}
			}
		}

		/**
		 * Destructor
		 */
		virtual ~ApiMsgIqrfStandardFrc() {};

		/**
		 * Checks if FRC extra result DPA request should be sent
		 * @return true if FRC extra result DPA request should be sent, false otherwise
		 */
		bool getExtraResult() const { return m_getExtraResult; }

		/**
		 * Checks if extended format should be used in response
		 * @return true if extended format should be used, false otherwise
		 */
		bool getExtFormat() const { return m_getExtFormat; }

		/**
		 * Stores FRC extra transaction result
		 * @param extraRes Transaction result
		 */
		void setDpaTransactionExtraResult(std::unique_ptr<IDpaTransactionResult2> extraRes) {
			m_extraRes = std::move(extraRes);
		}

	protected:
		/**
		 * Populates response document with data
		 * @param doc Response document
		 */
		void createResponsePayload(Document& doc) override {
			ApiMsgIqrfStandard::createResponsePayload(doc);

			if (getStatus() == 0) {
				if (hasSensorIndex) {
					Pointer("/data/rsp/result/sensorIndex").Set(doc, sensorIndex);
				}

				if (!m_selectedNodes.IsNull()) {
					Pointer("/data/rsp/result/selectedNodes").Set(doc, m_selectedNodes);
				}
			}
			bool r = (bool)m_extraRes;
			if (getVerbose() && r) {
				Pointer("/data/raw/1/request").Set(doc, r ? encodeBinary(m_extraRes->getRequest().DpaPacket().Buffer, m_extraRes->getRequest().GetLength()) : "");
				Pointer("/data/raw/1/requestTs").Set(doc, r ? TimeConversion::encodeTimestamp(m_extraRes->getRequestTs()) : "");
				Pointer("/data/raw/1/confirmation").Set(doc, r ? encodeBinary(m_extraRes->getConfirmation().DpaPacket().Buffer, m_extraRes->getConfirmation().GetLength()) : "");
				Pointer("/data/raw/1/confirmationTs").Set(doc, r ? TimeConversion::encodeTimestamp(m_extraRes->getConfirmationTs()) : "");
				Pointer("/data/raw/1/response").Set(doc, r ? encodeBinary(m_extraRes->getResponse().DpaPacket().Buffer, m_extraRes->getResponse().GetLength()) : "");
				Pointer("/data/raw/1/responseTs").Set(doc, r ? TimeConversion::encodeTimestamp(m_extraRes->getResponseTs()) : "");
			}
		}

	private:
		/// Controls whether FRC extra result is sent to collect the remaining bytes of data
		bool m_getExtraResult = true;
		/// Get device address
		bool m_getNadr = false;
		/// Return extended response format
		bool m_getExtFormat = false;
		/// FRC extra result DPA message
		DpaMessage m_dpaRequestExtra;
		/// Pointer to store FRC extra result transaction
		std::unique_ptr<IDpaTransactionResult2> m_extraRes;
		/// Selected nodes
		std::set<uint8_t> selectedNodes;
		/// Sensor index present in request
		bool hasSensorIndex = false;
		/// Sensor index
		uint8_t sensorIndex = 0;
		/// Selected nodes document
		rapidjson::Document m_selectedNodes;
	};

}
