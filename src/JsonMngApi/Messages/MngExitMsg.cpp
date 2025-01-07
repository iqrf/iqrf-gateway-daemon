/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#include "MngExitMsg.h"

namespace iqrf {

	MngExitMsg::MngExitMsg(const rapidjson::Document &doc) : ApiMsg(doc) {
		const auto *val = rapidjson::Pointer("/data/req/timeToExit").Get(doc);
		if (val && val->IsUint()) {
			m_timeToExit = val->GetUint();
		}
	}

	uint32_t MngExitMsg::getExitTime() const {
		return m_timeToExit;
	}

	void MngExitMsg::createResponsePayload(rapidjson::Document &doc) {
		if (m_timeToExit > 0) {
			rapidjson::Pointer("/data/rsp/timeToExit").Set(doc, m_timeToExit);
		}
	}
}
