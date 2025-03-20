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

#include "EnumerateMsg.h"

namespace iqrf {

	EnumerateMsg::EnumerateMsg(const Document &doc) : BaseMsg(doc) {
		const Value *v = Pointer("/data/req/reenumerate").Get(doc);
		if (v) {
			parameters.reenumerate = v->GetBool();
		}
		v = Pointer("/data/req/standards").Get(doc);
		if (v) {
			parameters.standards = v->GetBool();
		}
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

	void EnumerateMsg::createErrorResponsePayload(Document &doc) {
		Pointer("/mType").Set(doc, getMType());
		Pointer("/data/msgId").Set(doc, getMsgId());
		BaseMsg::createResponsePayload(doc);
		if (getVerbose()) {
			Pointer("/data/insId").Set(doc, getInsId());
			Pointer("/data/statusStr").Set(doc, getStatusStr());
		}
		Pointer("/data/status").Set(doc, getStatus());
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
