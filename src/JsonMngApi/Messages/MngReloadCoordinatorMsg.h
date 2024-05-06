/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

#include "IIqrfDpaService.h"
#include "IIqrfNetworkEnum.h"

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
		 * @param networkEnumService network enum service interface
		 */
		MngReloadCoordinatorMsg(const Document &doc, IIqrfDpaService *dpaService, IIqrfNetworkEnum *networkEnumService);

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
		/// Network enum service
		IIqrfNetworkEnum *m_networkEnumService = nullptr;
	};
}
