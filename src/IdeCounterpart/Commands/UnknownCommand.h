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

#include "BaseCommand.h"

/// iqrf namespace
namespace iqrf {

	/// Unknown command class
	class UnknownCommand : public BaseCommand {
	public:
		/**
		 * Delete default constructor
		 */
		UnknownCommand() = delete;

		/**
		 * Full constructor
		 * @param message UDP message
		 */
		UnknownCommand(const std::basic_string<uint8_t> &message) : BaseCommand(message) {};

		/**
		 * Destructor
		 */
		virtual ~UnknownCommand() {};

		/**
		 * Builds and encodes unknown or unsupported command response
		 */
		void buildResponse() override {
			m_header[CMD] |= PACKET_RESPONSE_CODE;
			m_header[SUBCMD] = PACKET_ERROR;

			encodeResponse();
		}
	};
}
