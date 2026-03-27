/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
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
   * Parses an ISO8601 timestamp string into a `time_point` object from chrono library
   *
   * The method accepts timestamp strings in UTC as well as other time zones,
   * the expected format is therefore the following:
   * YYYY-MM-DDTHH:MM:SSZ
   * YYYY-MM-DDTHH:MM:SS+HH:MM
   * YYYY-MM-DDTHH:MM:SS-HH:MM
   *
   * @param timestamp Timestamp to parse
   * @return `std::chrono::system_clock::time_point` chrono time point representation of timestamp
   * @throws `std::invalid_argument` Thrown if input timestamp string is not in format of expected ISO8601 string
   */
  static std::chrono::system_clock::time_point parseISO8601(
    const std::string &timestamp
  ) {
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

  /**
   * Converts a `time_point` from chrono library to ISO8601 string
   *
   * The method returns a string representation of a time point the following format:
   * YYYY-MM-DDTHH:MM:SSZ
   *
   * @param tp Time point
   * @return `std::string` ISO8601 timestamp string
   */
  static std::string toISO8601(const std::chrono::system_clock::time_point& tp) {
    auto tp_sec = date::floor<std::chrono::milliseconds>(tp);
    return date::format("%FT%TZ", tp_sec);
  }
};
