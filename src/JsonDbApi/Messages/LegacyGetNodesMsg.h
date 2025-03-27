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
	 * Get nodes request message
	 */
	class LegacyGetNodesMsg : public BaseMsg {
	public:
		/// Delete base constructor
		LegacyGetNodesMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		LegacyGetNodesMsg(const Document &doc) : BaseMsg(doc) {}

		/**
		 * Handles get devices request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override {
			includeMetadata = dbService->getMetadataToMessages();
			devices = dbService->getDevices();
		}

		/**
		 * Populates response document with devices response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			Value array(kArrayType);
			Document::AllocatorType &allocator = doc.GetAllocator();

			for (auto &[device, product] : devices) {
				Value object(kObjectType);

				Pointer("/nAdr").Set(object, device.getAddress(), allocator);
				Pointer("/mid").Set(object, device.getMid(), allocator);
				Pointer("/disc").Set(object, device.isDiscovered(), allocator);
				Pointer("/hwpid").Set(object, product.getHwpid(), allocator);
				Pointer("/hwpidVer").Set(object, product.getHwpidVersion(), allocator);
				Pointer("/osBuild").Set(object, product.getOsBuild(), allocator);
				Pointer("/dpaVer").Set(object, product.getDpaVersion(), allocator);
				if (includeMetadata) {
					std::shared_ptr<std::string> metadata = device.getMetadata();
					if (metadata) {
						Document metadataDoc;
						metadataDoc.Parse(metadata.get()->c_str());
						object.AddMember("metaData", rapidjson::Value(metadataDoc, allocator).Move(), allocator);
					} else {
						object.AddMember("metaData", rapidjson::Value(kNullType), allocator);
					}
				}

				array.PushBack(object, allocator);
			}

			Pointer("/data/rsp/nodes").Set(doc, array, allocator);
			BaseMsg::createResponsePayload(doc);
		}
	private:
		/// Include metadata in response
		bool includeMetadata = false;
		/// Vector of devices with product information
		std::vector<std::pair<Device, Product>> devices;
	};
}
