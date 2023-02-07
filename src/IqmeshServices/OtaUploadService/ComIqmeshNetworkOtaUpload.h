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

#include "ComBase.h"
#include "Trace.h"
#include <list>

using namespace rapidjson;

namespace iqrf {

	// OtaUpload input parameters
	typedef struct TOtaUploadInputParams {
		TOtaUploadInputParams()
		{
			hwpId = HWPID_DoNotCheck;
			repeat = 1;
			uploadEepromData = false;
			uploadEeepromData = false;
		}
		uint16_t deviceAddress;
		uint16_t hwpId;
		std::set<uint8_t> selectedNodes;
		std::string fileName;
		uint16_t repeat;
		uint16_t startMemAddr;
		bool uploadEepromData;
		bool uploadEeepromData;
	} TOtaUploadInputParams;

	class ComIqmeshNetworkOtaUpload : public ComBase {
	public:
		ComIqmeshNetworkOtaUpload() = delete;
		explicit ComIqmeshNetworkOtaUpload(Document& doc) : ComBase(doc) {
			parse(doc);
		}

		virtual ~ComIqmeshNetworkOtaUpload() {}

		const TOtaUploadInputParams getOtaUploadInputParams() const {
			return m_otaUploadInputParams;
		}

	protected:
		void createResponsePayload(Document& doc, const IDpaTransactionResult2& res) override {
			Pointer("/data/rsp/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
		}

	private:
		TOtaUploadInputParams m_otaUploadInputParams;

		void parse(rapidjson::Document& doc) {
			const Value* val = Pointer("/data/repeat").Get(doc);
			if (val && val->IsInt()) {
				m_otaUploadInputParams.repeat = static_cast<uint16_t>(val->GetInt());
			}

			val = Pointer("/data/req/deviceAddr").Get(doc);
			if (val && val->IsInt()) {
				m_otaUploadInputParams.deviceAddress = static_cast<uint16_t>(val->GetInt());
			}

			val = Pointer("/data/req/hwpId").Get(doc);
			if (val && val->IsInt()) {
				m_otaUploadInputParams.hwpId = static_cast<uint16_t>(val->GetInt());
			}

			val = Pointer("/data/req/selectedNodes").Get(doc);
			if (val && val->IsArray()) {
				for (const Value *itr = val->Begin(); itr != val->End(); ++itr) {
					m_otaUploadInputParams.selectedNodes.insert(static_cast<uint8_t>(itr->GetUint()));
				}
			}

			val = Pointer("/data/req/fileName").Get(doc);
			if (val && val->IsString()) {
				m_otaUploadInputParams.fileName = val->GetString();
			}

			val = Pointer("/data/req/startMemAddr").Get(doc);
			if (val && val->IsInt()) {
				m_otaUploadInputParams.startMemAddr = static_cast<uint16_t>(val->GetInt());
			}

			val = Pointer("/data/req/uploadEepromData").Get(doc);
			if (val && val->IsBool()) {
				m_otaUploadInputParams.uploadEepromData = val->GetBool();
			}

			val = Pointer("/data/req/uploadEeepromData").Get(doc);
			if (val && val->IsBool()) {
				m_otaUploadInputParams.uploadEeepromData = val->GetBool();
			}
		}
	};
}