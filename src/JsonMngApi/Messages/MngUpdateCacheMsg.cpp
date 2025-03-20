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

#include "MngUpdateCacheMsg.h"

namespace iqrf {

	MngUpdateCacheMsg::MngUpdateCacheMsg(const Document &doc, IIqrfDb *infoService, IJsCacheService *cacheService) : MngBaseMsg(doc) {
		TRC_FUNCTION_ENTER("");
		m_dbService = infoService;
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
			m_dbService->reloadDrivers();
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
