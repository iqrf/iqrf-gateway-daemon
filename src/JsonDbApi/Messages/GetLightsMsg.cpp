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

#include "GetLightsMsg.h"

namespace iqrf {

	void GetLightsMsg::handleMsg(IIqrfDb *dbService) {
		lights = dbService->getLightAddresses();
	}

	void GetLightsMsg::createResponsePayload(Document &doc) {
		if (m_status == 0) {
			Value array(kArrayType);
			Document::AllocatorType &allocator = doc.GetAllocator();

			for (auto &item : lights) {
				array.PushBack(item, allocator);
			}

			Pointer("/data/rsp/lightDevices").Set(doc, array, allocator);
		}
		BaseMsg::createResponsePayload(doc);
	}
}
