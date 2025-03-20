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

#include "IqrfDbAux.h"

/// iqrf namespace
namespace iqrf {

	const std::string IqrfDbAux::date_format = "%a, %d %b %y %T %Z";

	std::vector<uint8_t> IqrfDbAux::selectNodes(const std::set<uint8_t> &nodes, const uint8_t &idx, const uint8_t &count) {
		std::vector<uint8_t> selectedNodes(30, 0);
		std::set<uint8_t>::iterator it = nodes.begin();
		std::advance(it, idx);
		for (std::set<uint8_t>::iterator end = std::next(it, count); it != end; it++) {
			selectedNodes[*it / 8] |= (1 << (*it % 8));
		}
		return selectedNodes;
	}

	std::shared_ptr<std::string> IqrfDbAux::getCurrentTimestamp() {
		char datetime[200];
		time_t t = time(NULL);
		std::tm *tmp = std::gmtime(&t);

		std::time(NULL);
		std::strftime(datetime, sizeof(datetime), date_format.c_str(), tmp);
		return std::make_shared<std::string>(datetime);
	}
}
