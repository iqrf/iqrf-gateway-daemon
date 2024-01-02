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

#include "BaseCommand.h"

/// iqrf namespace
namespace iqrf {

	/// TR reset command class
	class TrReset : public BaseCommand {
	public:
		/**
		 * Delete default constructor
		 */
		TrReset() = delete;

		/**
		 * Full constructor
		 * @param message DPA message
		 * @param exclusiveAccess Exclusive access acquired
		 */
		TrReset(const std::basic_string<uint8_t> &message, bool exclusiveAccess) : BaseCommand(message), m_exclusiveAccess(exclusiveAccess) {
			m_trWrite = true;
		}

		/**
		 * Destructor
		 */
		virtual ~TrReset(){};

		/**
		 * Returns TR reset DPA request
		 * @return TR reset DPA request
		 */
		static std::basic_string<unsigned char> getDpaRequest() {
			return std::basic_string<unsigned char>{0x00, 0x00, 0x02, 0x01, 0xff, 0xff};
		}

		/**
		 * Builds and encodes TR reset response
		 */
		void buildResponse() override {
			m_response = m_header;
			if (m_exclusiveAccess) {
				m_response[SUBCMD] = PACKET_OK;
			} else {
				m_response[SUBCMD] = PACKET_ERROR;
			}

			encodeResponse();
		}

	private:
		/// Exclusive access acquired
		bool m_exclusiveAccess = false;
	};
}
