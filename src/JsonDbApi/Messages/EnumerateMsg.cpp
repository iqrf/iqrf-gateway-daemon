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

#include "EnumerateMsg.h"

namespace iqrf {

	EnumerateMsg::EnumerateMsg(const Document &doc) : BaseMsg(doc) {
		const Value *v = Pointer("/data/req/reenumerate").Get(doc);
		if (v) {
			parameters.reenumerate = v->GetBool();
		}
		parameters.standards = Pointer("/data/req/standards").Get(doc)->GetBool();
	}

	void EnumerateMsg::handleMsg(IIqrfDb *dbService) {
		dbService->enumerate(parameters);
	}

	void EnumerateMsg::createResponsePayload(Document &doc) {
		Document::AllocatorType &allocator = doc.GetAllocator();
		Pointer("/data/rsp/step").Set(doc, stepCode, allocator);
		Pointer("/data/rsp/stepStr").Set(doc, stepStr, allocator);
		BaseMsg::createResponsePayload(doc);
	}

	void EnumerateMsg::setStepCode(const uint8_t &stepCode) {
		this->stepCode = stepCode;
	}

	void EnumerateMsg::setStepString(const std::string &stepStr) {
		this->stepStr = stepStr;
	}

	void EnumerateMsg::setFinished() {
		finished = true;
	}
}
