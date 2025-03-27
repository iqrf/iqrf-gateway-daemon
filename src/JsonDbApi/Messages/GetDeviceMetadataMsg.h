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
	 * Database store device metadata request message
	 */
	class GetDeviceMetadataMsg : public BaseMsg {
	public:
		/// Delete base constructor
		GetDeviceMetadataMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		GetDeviceMetadataMsg(const Document &doc) : BaseMsg(doc) {
			const Value &requestArray = doc["data"]["req"]["devices"];
			for (SizeType i = 0; i < requestArray.Size(); ++i) {
				requestedDevices.insert(requestArray[i].GetInt());
			}
		}

		/**
		 * Handle reset request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override {
			for (auto &item : requestedDevices) {
				try {
					auto metadata = dbService->getDeviceMetadata(item);
					deviceMetadata.insert(std::make_pair(item, std::make_tuple(true, metadata)));
				} catch (const std::logic_error &e) {
					deviceMetadata.insert(std::make_pair(item, std::make_tuple(false, std::make_shared<std::string>(e.what()))));
				}
			}
		}

		/**
		 * Populates response document with sensors response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			if (m_status == 0) {
				Value array(kArrayType);
				Document::AllocatorType &allocator = doc.GetAllocator();

				for (auto item : deviceMetadata) {
					Value deviceObject;
					bool status = std::get<0>(item.second);

					Pointer("/address").Set(deviceObject, item.first, allocator);
					Pointer("/success").Set(deviceObject, status, allocator);
					if (status) {
						Document metadataDoc(kObjectType);
						auto str = std::get<1>(item.second);
						if (str) {
							metadataDoc.Parse(str.get()->c_str());
							deviceObject.AddMember("metadata", rapidjson::Value(metadataDoc, allocator).Move(), allocator);
						} else {
							deviceObject.AddMember("metadata", rapidjson::Value(kNullType), allocator);
						}
					} else {
						auto str = std::get<1>(item.second)->c_str();
						Pointer("/errorStr").Set(deviceObject, str, allocator);
					}
					array.PushBack(deviceObject, allocator);
				}

				Pointer("/data/rsp/devices").Set(doc, array, allocator);
			}
			BaseMsg::createResponsePayload(doc);
		}
	private:
		/// Map of device addresses and metadata
		std::set<uint8_t> requestedDevices;
		/// Response document
		std::map<uint8_t, GetMetadataResponse> deviceMetadata;
	};
}
