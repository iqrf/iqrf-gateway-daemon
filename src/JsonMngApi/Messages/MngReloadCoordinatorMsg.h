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

#include "IIqrfDpaService.h"
#include "IIqrfDb.h"

namespace iqrf {

	/**
	 * Reload coordinator messages
	 */
	class MngReloadCoordinatorMsg : public MngBaseMsg {
	public:
		/// Delete base constructor
		MngReloadCoordinatorMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 * @param dpaService DPA service interface
		 * @param dbService DB service interface
		 */
		MngReloadCoordinatorMsg(const Document &doc, IIqrfDpaService *dpaService, IIqrfDb *dbService);

		/**
		 * Destructor
		 */
		virtual ~MngReloadCoordinatorMsg() {};

		/**
		 * Handles coordinator reload message
		 */
		void handleMsg() override;
	private:
		/// DPA service interface
		IIqrfDpaService *m_dpaService = nullptr;
		/// DB service interface
		IIqrfDb *m_dbService = nullptr;
	};
}
