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

#include "BaseMsg.h"

namespace iqrf {

	void BaseMsg::createResponsePayload(Document &doc) {
		Value *v = Pointer("/data/rsp").Get(doc);
		if (!v) {
			Value empty;
			empty.SetObject();
			Pointer("/data/rsp").Set(doc, empty);
		}
	}

	const std::string& BaseMsg::getMessagingId() const {
		return messagingId;
	}

	void BaseMsg::setMessagingId(const std::string &messagingId) {
		this->messagingId = messagingId;
	}

}
