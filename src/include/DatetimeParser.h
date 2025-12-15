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

#include <date/date.h>
#include <sstream>

/**
 * ISO 8601 date-time timestamp parser
 */
class DatetimeParser {
public:
	/**
	 * Converts ISO8601 date-time timestamp string to time point
	 * @param timestamp Timestamp string
	 * @return const std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> Time point
	 */
	static const std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> parse_to_timepoint(const std::string &timestamp) {
    std::istringstream in{timestamp};
    date::sys_time<std::chrono::milliseconds> tp;
    if (!timestamp.empty() && timestamp.back() == 'Z') {
      in >> date::parse("%FT%TZ", tp);
    } else {
      in >> date::parse("%FT%T%Ez", tp);
    }
    if (!in.fail()) {
      return tp;
    }
    throw std::invalid_argument("Invalid datetime string format.");
	}
};
