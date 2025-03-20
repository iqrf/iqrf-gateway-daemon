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
		 * Destructor
		 */
		virtual ~BaseMsg() {};

		/**
		 * Prepares response document
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override;

		/**
		 * Returns messaging instance
		 * @return Messaging instance
		 */
		const std::shared_ptr<MessagingInstance>& getMessaging() const;

		/**
		 * Sets messaging instance
		 * @param messagingId Messaging instance
		 */
		void setMessaging(const MessagingInstance &messaging);

		/**
		 * Message handler method
		 */
		virtual void handleMsg(IIqrfDb *dbService) = 0;
	private:
		/// Messaging instance
		std::shared_ptr<MessagingInstance> messaging = nullptr;
	};
}
