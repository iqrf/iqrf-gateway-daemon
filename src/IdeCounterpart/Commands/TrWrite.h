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

#include "BaseCommand.h"
#include "IIqrfChannelService.h"

/// iqrf namespace
namespace iqrf {
	/// TR write status code enum
	enum TrWriteResult {
		OK = 0x50,
		ERROR_LEN = 0x60,
		ERROR_BUSY = 0x61,
		ERROR_CRC = 0x62,
		ERROR_SERVICE = 0x63,
	};

	/// TR write command class
	class TrWrite : public BaseCommand {
	public:
		/**
		 * Delete default constructor
		 */
		TrWrite() = delete;

		/**
		 * Full constructor
		 * @param message UDP message
		 * @param exclusiveAccess Exclusive access acquired
		 */
		TrWrite(const std::basic_string<uint8_t> &message, bool exclusiveAccess) : BaseCommand(message), m_exclusiveAccess(exclusiveAccess) {
			m_trWrite = true;
		}

		/**
		 * Destructor
		 */
		virtual ~TrWrite(){};

		/**
		 * Builds and encodes TR write response
		 */
		void buildResponse() override {
			m_response = m_header;
			if (m_exclusiveAccess) {
				m_response[SUBCMD] = TrWriteResult::OK;
			} else {
				m_response[SUBCMD] = TrWriteResult::ERROR_SERVICE;
			}

			encodeResponse();
		}
	private:
		/// Exclusive access acquired
		bool m_exclusiveAccess = false;
	};
}
