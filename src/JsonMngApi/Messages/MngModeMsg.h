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

#include "IUdpConnectorService.h"

namespace iqrf {

	/**
	 * Daemon mode request message
	 */
	class MngModeMsg : public MngBaseMsg {
	public:
		/// Delete base constructor
		MngModeMsg() = delete;

		/**
		 * Request document constructor
		 * @param doc Request document
		 * @param udpConnectorService UDP connector service interface
		 */
		MngModeMsg(const Document &doc, IUdpConnectorService *udpConnectorService);

		/**
		 * Destructor
		 */
		virtual ~MngModeMsg() {};

		/**
		 * Handles daemon mode request
		 */
		void handleMsg() override;

		/**
		 * Populates response document
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override;
	private:
		/// UDP connector service interface
		IUdpConnectorService *m_udpConnectorService = nullptr;
		/// Daemon mode
		IUdpConnectorService::Mode m_mode;
	};
}
