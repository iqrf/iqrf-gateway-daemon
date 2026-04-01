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

#include "BaseMsg.h"

using namespace rapidjson;

namespace iqrf {

	/**
	 * Get binary outputs request message
	 */
	class LegacyGetBinaryOutputsMsg : public BaseMsg {
	public:
		/// Delete base constructor
		LegacyGetBinaryOutputsMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		LegacyGetBinaryOutputsMsg(const Document &doc) : BaseMsg(doc) {};

		/**
		 * Handles get binary outputs request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override {
			includeMetadata = dbService->getMetadataToMessages();
			bos = dbService->getBinaryOutputCountMap();
			for (const auto &[address, _] : bos) {
				metadata.insert_or_assign(
					address,
					dbService->getDeviceMetadata(address)
				);
			}
		}

		/**
		 * Populates response document with binary outputs response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			Value array(kArrayType);
			Document::AllocatorType &allocator = doc.GetAllocator();

			for (const auto &[address, count] : bos) {
				Value object;
				Pointer("/nAdr").Set(object, address, allocator);
				Pointer("/binOuts").Set(object, count, allocator);

				if (includeMetadata) {
					if (metadata.count(address) == 0 || metadata[address] == nullptr) {
						object.AddMember("metaData", rapidjson::Value(kNullType), allocator);
					} else {
						Document metadataDoc(kObjectType);
						metadataDoc.Parse(metadata[address]->c_str());
						object.AddMember("metaData", rapidjson::Value(metadataDoc, allocator).Move(), allocator);
					}
				}

				array.PushBack(object, allocator);
			}

			Pointer("/data/rsp/binOutDevices").Set(doc, array, allocator);
			BaseMsg::createResponsePayload(doc);
		}
	private:
		/// Include metadata in response
		bool includeMetadata = false;
		/// Map of device addresses and number of implemented binary outputs
		std::map<uint8_t, uint8_t> bos;
		/// Map of device addresses and metadata
		std::map<uint8_t, std::shared_ptr<std::string>> metadata;
	};
}
