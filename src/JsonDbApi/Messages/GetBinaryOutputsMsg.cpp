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

#include "GetBinaryOutputsMsg.h"

namespace iqrf {

	void GetBinaryOutputsMsg::handleMsg(IIqrfDb *dbService) {
		bos = dbService->getBinaryOutputCountMap();
	}

	void GetBinaryOutputsMsg::createResponsePayload(Document &doc) {
		if (m_status == 0) {
			Value array(kArrayType);
			Document::AllocatorType &allocator = doc.GetAllocator();

			for (auto &item : bos) {
				Value object;
				Pointer("/address").Set(object, item.first, allocator);
				Pointer("/count").Set(object, item.second, allocator);
				array.PushBack(object, allocator);
			}

			Pointer("/data/rsp/binoutDevices").Set(doc, array, allocator);
		}
		BaseMsg::createResponsePayload(doc);
	}
}
