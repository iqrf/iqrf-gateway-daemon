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
#include <set>

using namespace rapidjson;

namespace iqrf {

	/**
	 * Get lights request message
	 */
	class LegacyGetLightsMsg : public BaseMsg {
	public:
		/// Delete base constructor
		LegacyGetLightsMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		LegacyGetLightsMsg(const Document &doc) : BaseMsg(doc) {};

		/**
		 * Handles get lights request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override {
			includeMetadata = dbService->getMetadataToMessages();
			lights = dbService->getLightAddresses();
			for (const auto &address : lights) {
				metadata.insert_or_assign(
					address,
					dbService->getDeviceMetadata(address)
				);
			}
		}

		/**
		 * Populates response document with lights response
		 */
		void createResponsePayload(Document &doc) override {
			Value array(kArrayType);
			Document::AllocatorType &allocator = doc.GetAllocator();

			for (const auto &address : lights) {
				Value object(kObjectType);
				Pointer("/nAdr").Set(object, address, allocator);

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

			Pointer("/data/rsp/lightDevices").Set(doc, array, allocator);
			BaseMsg::createResponsePayload(doc);
		}
	private:
		/// Include metadata in response
		bool includeMetadata = false;
		/// Set of device addresses implementing lights standard
		std::set<uint8_t> lights;
		/// Map of device addresses and metadata
		std::map<uint8_t, std::shared_ptr<std::string>> metadata;
	};
}
