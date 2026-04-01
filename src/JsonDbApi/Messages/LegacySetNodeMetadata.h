/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
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

#include <map>

#include "BaseMsg.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace iqrf {

	/**
	 * Database store device metadata request message
	 */
	class LegacySetNodeMetaDataMsg : public BaseMsg {
	public:
		/// Delete base constructor
		LegacySetNodeMetaDataMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		LegacySetNodeMetaDataMsg(const Document &doc) : BaseMsg(doc) {
			StringBuffer buffer;
			Writer<StringBuffer> writer(buffer);

			address = static_cast<uint8_t>(Pointer("/data/req/nAdr").Get(doc)->GetUint());
			const Value* metadataVal = Pointer("/data/req/metaData").Get(doc);
			if (!metadataVal->IsNull()) {
				metadataVal->Accept(writer);
				metadata = std::make_shared<std::string>(buffer.GetString());
			}
		}

		/**
		 * Handle reset request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override {
			dbService->setDeviceMetadata(address, metadata);
		}

		/**
		 * Populates response document with sensors response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			Value object(kObjectType);
			Document::AllocatorType &allocator = doc.GetAllocator();

			Pointer("/nAdr").Set(object, address, allocator);
			if (metadata) {
				Document metadataDoc(kObjectType);
				metadataDoc.Parse(metadata->c_str());
				object.AddMember("metaData", rapidjson::Value(metadataDoc, allocator).Move(), allocator);
			} else {
				object.AddMember("metaData", rapidjson::Value(kNullType), allocator);
			}

			Pointer("/data/rsp").Set(doc, object, allocator);
			BaseMsg::createResponsePayload(doc);
		}
	private:
		/// Node address
		uint8_t address;
		/// Metadata to set
		std::shared_ptr<std::string> metadata = nullptr;
	};
}
