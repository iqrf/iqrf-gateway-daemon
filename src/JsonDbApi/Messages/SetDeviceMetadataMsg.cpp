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

#include "SetDeviceMetadataMsg.h"

namespace iqrf {

	SetDeviceMetadataMsg::SetDeviceMetadataMsg(const Document &doc) : BaseMsg(doc) {
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);

		const Value &requestArray = doc["data"]["req"]["devices"];
		for (SizeType i = 0; i < requestArray.Size(); ++i) {
			uint8_t address = requestArray[i]["address"].GetInt();
			if (requestArray[i]["metadata"].IsNull()) {
				metadataRequest.insert(std::make_pair(address, nullptr));
			} else {
				requestArray[i]["metadata"].Accept(writer);
				std::string metadata = buffer.GetString();
				metadataRequest.insert(std::make_pair(address, std::make_shared<std::string>(metadata)));
				buffer.Clear();
				writer.Reset(buffer);
			}
		}
	}

	void SetDeviceMetadataMsg::handleMsg(IIqrfDb *dbService) {
		for (auto &item : metadataRequest) {
			try {
				dbService->setDeviceMetadata(item.first, item.second);
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
