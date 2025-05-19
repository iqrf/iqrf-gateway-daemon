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
