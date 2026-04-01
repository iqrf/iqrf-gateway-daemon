/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
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
