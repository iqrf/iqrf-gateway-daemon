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

#include "GetNetworkTopologyMsg.h"

namespace iqrf {

	void GetNetworkTopologyMsg::handleMsg(IIqrfDb *dbService) {
		devices = dbService->getDevicesWithProductInfo();
	}

	void GetNetworkTopologyMsg::createResponsePayload(Document &doc) {
		if (m_status == 0) {
			Value array(kArrayType);
			Document::AllocatorType &allocator = doc.GetAllocator();

			for (auto &item : devices) {
				Value object;
				Device device = std::get<0>(item);
				Pointer("/address").Set(object, device.getAddress(), allocator);
				Pointer("/vrn").Set(object, device.getVrn(), allocator);
				Pointer("/zone").Set(object, device.getZone(), allocator);
				std::shared_ptr<uint8_t> val = device.getParent();
				if (val) {
					Pointer("/parent").Set(object, *val.get(), allocator);
				} else {
					Pointer("/parent").Create(object, allocator);
				}
				Pointer("/os").Set(object, std::get<4>(item), allocator);
				Pointer("/dpa").Set(object, std::get<5>(item), allocator);

				array.PushBack(object, allocator);
			}

			Pointer("/data/rsp/devices").Set(doc, array, allocator);
		}
		BaseMsg::createResponsePayload(doc);
	}
}
