/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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

#include "MngBaseMsg.h"

#include "IIqrfInfo.h"
#include "IJsCacheService.h"

namespace iqrf {

	/**
	 * JsCache update request message
	 */
	class MngUpdateCacheMsg : public MngBaseMsg {
	public:
		/// Delete base constructor
		MngUpdateCacheMsg() = delete;

		/**
		 * Request document constructor
		 * @param doc Request document
		 * @param infoService IQRF Info service interface
		 * @param cacheService JS cache service interface
		 */
		MngUpdateCacheMsg(const Document &doc, IIqrfInfo *infoService, IJsCacheService *cacheService);

		/**
		 * Destructor
		 */
		virtual ~MngUpdateCacheMsg() {};

		/**
		 * Handles cache update request
		 */
		void handleMsg() override;

		/**
		 * Populates response document
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override;
	private:
		/// Info service interface
		IIqrfInfo *m_infoService = nullptr;
		/// Cache service interface
		IJsCacheService *m_cacheService = nullptr;
		/// Cache update status
		IJsCacheService::CacheStatus m_cacheState = IJsCacheService::CacheStatus::UPDATE_NEEDED;
	};
}
