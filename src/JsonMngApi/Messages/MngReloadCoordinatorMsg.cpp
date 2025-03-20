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

#include "MngReloadCoordinatorMsg.h"

namespace iqrf {

	MngReloadCoordinatorMsg::MngReloadCoordinatorMsg(const Document &doc, IIqrfDpaService *dpaService, IIqrfDb *dbService) : MngBaseMsg(doc) {
		m_dpaService = dpaService;
		m_dbService = dbService;
	}

	void MngReloadCoordinatorMsg::handleMsg() {
		m_dpaService->reinitializeCoordinator();
		m_dbService->reloadDrivers();
	}
}
