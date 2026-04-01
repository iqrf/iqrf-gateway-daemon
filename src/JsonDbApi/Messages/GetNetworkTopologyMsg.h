/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
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
#pragma once

#include "BaseMsg.h"

using namespace rapidjson;

namespace iqrf {

	/**
	 * Get network topology request message
	 */
	class GetNetworkTopologyMsg : public BaseMsg {
	public:
		/// Delete base constructor
		GetNetworkTopologyMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		GetNetworkTopologyMsg(const Document &doc) : BaseMsg(doc) {};

		/**
		 * Handles get network topology request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override {
			devices = dbService->getDevices();
		}

		/**
		 * Populates response document with network topology response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			if (m_status == 0) {
				Value array(kArrayType);
				Document::AllocatorType &allocator = doc.GetAllocator();

				for (auto &[device, product] : devices) {
					Value object;
					Pointer("/address").Set(object, device.getAddress(), allocator);
					Pointer("/vrn").Set(object, device.getVrn(), allocator);
					Pointer("/zone").Set(object, device.getZone(), allocator);
					if (device.getParent().has_value()) {
						Pointer("/parent").Set(object, device.getParent().value(), allocator);
					} else {
						Pointer("/parent").Create(object, allocator);
					}
					Pointer("/os").Set(object, product.getOsVersion(), allocator);
					Pointer("/dpa").Set(object, product.getDpaVersion(), allocator);

					array.PushBack(object, allocator);
				}

				Pointer("/data/rsp/devices").Set(doc, array, allocator);
			}
			BaseMsg::createResponsePayload(doc);
		}
	private:
		/// Vector of devices with product information
		std::vector<std::pair<Device, Product>> devices;
	};
}
