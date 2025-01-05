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
#pragma once

#include "MngBaseMsg.h"
#include "VersionInfo.h"

namespace iqrf {

	class MngVersionMsg : public MngBaseMsg {
	public:
		/// Delete base constructor
		MngVersionMsg() = delete;

		/**
		 * Request document constructor
		 * @param doc Request document
		 */
		MngVersionMsg(const Document &doc) : MngBaseMsg(doc) {};

		/**
		 * Destructor
		 */
		virtual ~MngVersionMsg() {};

		/**
		 * Handles version request message
		 */
		void handleMsg() override;

		/**
		 * Populates response document
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override;
	private:
		/// Daemon version string
		std::string m_daemonVersion;
	};
}
