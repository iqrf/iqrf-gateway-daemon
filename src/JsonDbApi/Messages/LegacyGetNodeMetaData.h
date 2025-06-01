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
