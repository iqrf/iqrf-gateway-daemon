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

#include "IIqrfDpaService.h"
//#include "RawDpaEmbedCoordinator.h"

#include <cstdint>
#include <ctime>
#include <set>
#include <vector>

/// iqrf namespace
namespace iqrf {

	/**
	 * IQRF DB auxiliary functions class
	 */
	class IqrfDbAux {
	public:
		/**
		 * Populates selected nodes for selective FRC requests
		 * @param nodes Nodes
		 * @param idx Index to start from
		 * @param count Number of nodes to add
		 * @return Selected nodes
		 */
		static std::vector<uint8_t> selectNodes(const std::set<uint8_t> &nodes, const uint8_t &idx, const uint8_t &count);

		/**
		 * Generates timestamp
		 * @return Timestamp string
		 */
		static std::shared_ptr<std::string> getCurrentTimestamp();
	private:
		/// Date and time string format
		static const std::string date_format;
	};
}
