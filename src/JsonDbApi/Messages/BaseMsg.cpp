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

	const std::shared_ptr<MessagingInstance>& BaseMsg::getMessaging() const {
		return messaging;
	}

	void BaseMsg::setMessaging(const MessagingInstance &messaging) {
		this->messaging = std::make_shared<MessagingInstance>(messaging);
	}

}
