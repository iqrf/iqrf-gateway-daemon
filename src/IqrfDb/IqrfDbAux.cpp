/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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
