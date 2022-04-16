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
#pragma once

#include "ApiMsg.h"
#include "IIqrfDb.h"

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
		 * Destructor
		 */
		virtual ~BaseMsg() {};

		/**
		 * Prepares response document
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override;

		/**
		 * Returns messaging ID
		 * @return Messaging ID
		 */
		const std::string& getMessagingId() const;

		/**
		 * Sets messaging ID
		 * @param messagingId Sets messaging ID
		 */
		void setMessagingId(const std::string &messagingId);

		/**
		 * Message handler method
		 */
		virtual void handleMsg(IIqrfDb *dbService) = 0;
	private:
		/// Messaging ID
		std::string messagingId;
	};
}
