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
