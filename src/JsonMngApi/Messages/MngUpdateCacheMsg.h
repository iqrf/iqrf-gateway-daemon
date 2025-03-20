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

#include "MngBaseMsg.h"

#include "IIqrfDb.h"
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
		 * @param dbService IQRF DB service interface
		 * @param cacheService JS cache service interface
		 */
		MngUpdateCacheMsg(const Document &doc, IIqrfDb *dbService, IJsCacheService *cacheService);

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
		/// DB service interface
		IIqrfDb *m_dbService = nullptr;
		/// Cache service interface
		IJsCacheService *m_cacheService = nullptr;
		/// Cache update status
		IJsCacheService::CacheStatus m_cacheState = IJsCacheService::CacheStatus::UPDATE_NEEDED;
	};
}
