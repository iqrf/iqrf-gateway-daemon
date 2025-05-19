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
#pragma once

#include "BaseMsg.h"

namespace iqrf {

	/**
	 * Enumerate request message
	 */
	class EnumerateMsg : public BaseMsg {
	public:
		/**
		 * Delete default constructor
		 */
		EnumerateMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		EnumerateMsg(const rapidjson::Document &doc) : BaseMsg(doc) {
			const Value *v = Pointer("/data/req/reenumerate").Get(doc);
			if (v) {
				parameters.reenumerate = v->GetBool();
			}
			v = Pointer("/data/req/standards").Get(doc);
			if (v) {
				parameters.standards = v->GetBool();
			}
		}

		/**
		 * Handle enumerate request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override {
			dbService->enumerate(parameters);
		}

		/**
		 * Populates response document with enumeration progress
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			Document::AllocatorType &allocator = doc.GetAllocator();
			Pointer("/data/rsp/step").Set(doc, stepCode, allocator);
			Pointer("/data/rsp/stepStr").Set(doc, stepStr, allocator);
			BaseMsg::createResponsePayload(doc);
		}

		/**
		 * Populates response document with enumeration error
		 * @param doc Response document
		 */
		void createErrorResponsePayload(Document &doc) {
			Pointer("/mType").Set(doc, getMType());
			Pointer("/data/msgId").Set(doc, getMsgId());
			BaseMsg::createResponsePayload(doc);
			if (getVerbose()) {
				Pointer("/data/insId").Set(doc, getInsId());
				Pointer("/data/statusStr").Set(doc, getStatusStr());
			}
			Pointer("/data/status").Set(doc, getStatus());
		}

		/**
		 * Sets enumeration step code
		 * @param stepCode Step code
		 */
		void setStepCode(const uint8_t &stepCode) {
			this->stepCode = stepCode;
		}

		/**
		 * Sets enumeration step message
		 * @param errorStr Step message
		 */
		void setStepString(const std::string &stepStr) {
			this->stepStr = stepStr;
		}

		/**
		 * Sets finished status of enumeration
		 */
		void setFinished() {
			finished = true;
		}
	private:
		/// Enumeration parameters
		IIqrfDb::EnumParams parameters;
		/// Step code
		uint8_t stepCode;
		/// Step string
		std::string stepStr;
		/// Enumeration finished
		bool finished = false;
	};
}
