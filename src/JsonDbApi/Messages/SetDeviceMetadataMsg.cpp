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

#include "SetDeviceMetadataMsg.h"

namespace iqrf {

	SetDeviceMetadataMsg::SetDeviceMetadataMsg(const Document &doc) : BaseMsg(doc) {
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);

		const Value &requestArray = doc["data"]["req"]["devices"];
		for (SizeType i = 0; i < requestArray.Size(); ++i) {
			uint8_t address = requestArray[i]["address"].GetInt();
			requestArray[i]["metadata"].Accept(writer);
			std::string metadata = buffer.GetString();
			metadataRequest.insert(std::make_pair(address, metadata));
			buffer.Clear();
			writer.Reset(buffer);
		}
	}

	void SetDeviceMetadataMsg::handleMsg(IIqrfDb *dbService) {
		for (auto &item : metadataRequest) {
			try {
				if (item.second == "{}") {
					metadataResponse.insert(std::make_pair(item.first, std::make_tuple(false, "Empty object not stored.")));
					continue;
				}
				dbService->setDeviceMetadata(item.first, std::make_shared<std::string>(item.second));
				metadataResponse.insert(std::make_pair(item.first, std::make_tuple(true, std::string())));
			} catch (const std::logic_error &e) {
				metadataResponse.insert(std::make_pair(item.first, std::make_tuple(false, e.what())));
			}
		}
	}

	void SetDeviceMetadataMsg::createResponsePayload(Document &doc) {
		Value array(kArrayType);
		Document::AllocatorType &allocator = doc.GetAllocator();

		for (auto &item : metadataResponse) {
			Value deviceObject;
			bool status = std::get<0>(item.second);
			Pointer("/address").Set(deviceObject, item.first, allocator);
			Pointer("/success").Set(deviceObject, status, allocator);
			if (!status) {
				Pointer("/errorStr").Set(deviceObject, std::get<1>(item.second), allocator);
			}
			array.PushBack(deviceObject, allocator);
		}

		Pointer("/data/rsp/devices").Set(doc, array, allocator);
		BaseMsg::createResponsePayload(doc);
	}
}
