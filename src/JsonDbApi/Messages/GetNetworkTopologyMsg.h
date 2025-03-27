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
