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


#include "MngUpdateCacheMsg.h"

namespace iqrf {

	MngUpdateCacheMsg::MngUpdateCacheMsg(const Document &doc, IIqrfInfo *infoService, IJsCacheService *cacheService) : MngBaseMsg(doc) {
		TRC_FUNCTION_ENTER("");
		m_infoService = infoService;
		m_cacheService = cacheService;
		TRC_FUNCTION_LEAVE("");
	}

	void MngUpdateCacheMsg::handleMsg() {
		TRC_FUNCTION_ENTER("");
		auto result = m_cacheService->invokeWorker();
		m_cacheState = std::get<0>(result);
		if (m_cacheState == IJsCacheService::CacheStatus::UPDATE_FAILED) {
			throw std::logic_error(std::get<1>(result));
		}
		if (m_cacheState == IJsCacheService::CacheStatus::UPDATED) {
			m_infoService->reloadDrivers();
		}
		TRC_FUNCTION_LEAVE("");
	}

	void MngUpdateCacheMsg::createResponsePayload(Document &doc) {
		TRC_FUNCTION_ENTER("");
		if (getStatus() == 0) {
			bool updated;
			std::string statusStr;
			if (m_cacheState == IJsCacheService::CacheStatus::UP_TO_DATE) {
				updated = false;
				statusStr = "Cache is up to date.";
			} else if (m_cacheState == IJsCacheService::CacheStatus::UPDATED) {
				updated = true;
				statusStr = "Cache has been updated.";
			}
			Pointer("/data/rsp/updated").Set(doc, updated);
			Pointer("/data/rsp/status").Set(doc, statusStr);
		}
		MngBaseMsg::createResponsePayload(doc);
		TRC_FUNCTION_LEAVE("");
	}
}
