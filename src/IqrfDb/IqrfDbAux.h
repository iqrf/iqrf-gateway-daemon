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
