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

#include "BaseMsg.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace iqrf {

	/**
	 * Database annotate metadata request message
	 */
	class MetadataAnnotation : public BaseMsg {
	public:
		/// Delete base constructor
		MetadataAnnotation() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		MetadataAnnotation(const Document &doc) : BaseMsg(doc) {
			annotate = Pointer("/data/req/annotate").Get(doc)->GetBool();
		}

		/**
		 * Handle reset request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override {
			dbService->setMetadataToMessages(annotate);
		}

		/**
		 * Populates response document with sensors response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			Document::AllocatorType &allocator = doc.GetAllocator();

			Value object(kObjectType);
			Pointer("/annotate").Set(object, annotate, allocator);

			Pointer("/data/rsp").Set(doc, object, allocator);
			BaseMsg::createResponsePayload(doc);
		}
	private:
		bool annotate = false;
	};
}
