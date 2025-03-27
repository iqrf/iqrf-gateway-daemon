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

#include "BaseMsg.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace iqrf {

	/**
	 * Database store device metadata request message
	 */
	class LegacySetMidMetaDataMsg : public BaseMsg {
	public:
		/// Delete base constructor
		LegacySetMidMetaDataMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		LegacySetMidMetaDataMsg(const Document &doc) : BaseMsg(doc) {
			StringBuffer buffer;
			Writer<StringBuffer> writer(buffer);

			mid = Pointer("/data/req/mid").Get(doc)->GetUint();
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
			auto device = dbService->getDeviceByMid(mid);
			if (!device) {
				throw std::runtime_error("Device with MID " + std::to_string(mid) + " does not exist");
			}
			device->setMetadata(metadata);
			dbService->updateDevice(*device);
		}

		/**
		 * Populates response document with sensors response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			Value object(kObjectType);
			Document::AllocatorType &allocator = doc.GetAllocator();

			Pointer("/mid").Set(object, mid, allocator);
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
		/// Node module ID
		uint32_t mid;
		/// Metadata to set
		std::shared_ptr<std::string> metadata = nullptr;
	};
}
