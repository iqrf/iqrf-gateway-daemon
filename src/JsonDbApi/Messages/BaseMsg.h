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

#include "ApiMsg.h"
#include "IIqrfDb.h"
#include "MessagingCommon.h"

using namespace rapidjson;

namespace iqrf {

	/**
	 * Base message class
	 */
	class BaseMsg : public ApiMsg {
	public:
		/**
		 * Base constructor
		 */
		BaseMsg() : ApiMsg() {};

		/**
		 * Full constructor
		 * @param doc Request document
		 */
		BaseMsg(const Document &doc) : ApiMsg(doc) {};

		/**
		 * Prepares response document
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			Value *v = Pointer("/data/rsp").Get(doc);
			if (!v) {
				Value empty;
				empty.SetObject();
				Pointer("/data/rsp").Set(doc, empty);
			}
		}

		/**
		 * Returns messaging instance
		 * @return Messaging instance
		 */
		const std::shared_ptr<MessagingInstance> getMessaging() const {
			return messaging;
		}

		/**
		 * Sets messaging instance
		 * @param messaging Messaging instance
		 */
		void setMessaging(const MessagingInstance& messaging) {
			this->messaging = std::make_shared<MessagingInstance>(messaging);
		}

		/**
		 * Message handler method
		 */
		virtual void handleMsg(IIqrfDb *dbService) = 0;
	private:
		/// Messaging instance
		std::shared_ptr<MessagingInstance> messaging = nullptr;
	};
}
