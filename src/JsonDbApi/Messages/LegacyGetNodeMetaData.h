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
#include <set>

#include "BaseMsg.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

typedef std::tuple<bool, std::shared_ptr<std::string>> GetMetadataResponse;

namespace iqrf {

	/**
	 * Database get node metadata request message
	 */
	class LegacyGetNodeMetaDataMsg : public BaseMsg {
	public:
		/// Delete base constructor
		LegacyGetNodeMetaDataMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		LegacyGetNodeMetaDataMsg(const Document &doc) : BaseMsg(doc) {
			address = static_cast<uint8_t>(Pointer("/data/req/nAdr").Get(doc)->GetUint());
		}

		/**
		 * Handle reset request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override {
			auto device = dbService->getDeviceByAddress(address);
			if (!device) {
				throw std::runtime_error("Device with address " + std::to_string(address) + " does not exist");
			}
			metadata = device->getMetadata();
		}

		/**
		 * Populates response document with sensors response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			Document::AllocatorType &allocator = doc.GetAllocator();

			Value object(kObjectType);
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
		/// Module ID
		uint8_t address = 0;
		/// Metadata
		std::shared_ptr<std::string> metadata;
	};
}
