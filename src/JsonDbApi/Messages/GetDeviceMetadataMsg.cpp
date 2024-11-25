/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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

#include "GetDeviceMetadataMsg.h"

namespace iqrf {

	GetDeviceMetadataMsg::GetDeviceMetadataMsg(const Document &doc) : BaseMsg(doc) {
		const Value &requestArray = doc["data"]["req"]["devices"];
		for (SizeType i = 0; i < requestArray.Size(); ++i) {
			requestedDevices.insert(requestArray[i].GetInt());
		}
	}

	void GetDeviceMetadataMsg::handleMsg(IIqrfDb *dbService) {
		for (auto &item : requestedDevices) {
			try {
				auto metadata = dbService->getDeviceMetadata(item);
				deviceMetadata.insert(std::make_pair(item, std::make_tuple(true, metadata)));
			} catch (const std::logic_error &e) {
				deviceMetadata.insert(std::make_pair(item, std::make_tuple(false, std::make_shared<std::string>(e.what()))));
			}
		}
	}

	void GetDeviceMetadataMsg::createResponsePayload(Document &doc) {
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
	}
}
