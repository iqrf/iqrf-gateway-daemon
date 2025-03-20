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

#include <date/date.h>
#include <regex>
#include <string_view>

/**
 * ISO 8601 date-time timestamp parser
 *
 * Based on: https://github.com/beached/iso8601_parsing
 */
class DatetimeParser {
public:
	/**
	 * Converts ISO8601 date-time timestamp string to time point
	 * @param timestamp Timestamp string
	 * @return const std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> Time point
	 */
	static const std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> parse_to_timepoint(std::string &timestamp) {
		if (!is_timestamp_valid(timestamp)) {
			throw std::logic_error("Invalid datetime string format.");
		}
		std::string_view sv = timestamp;
		const DateStruct date = parse_date(sv);
		if (is_special_character(sv)) {
			sv.remove_prefix(1);
		}
		const TimeStruct time = parse_time(sv);
		const int16_t offset = parse_offset(sv);
		std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp {
			date::sys_days{date::year(date.year) / date::month(date.month) / date::day(date.day)}
			+ std::chrono::hours(time.hours)
			+ std::chrono::minutes(time.minutes)
			+ std::chrono::seconds(time.seconds)
			+ std::chrono::milliseconds(time.milliseconds)
		};
		tp -= std::chrono::minutes(offset);
		return tp;
	}
private:
	/**
	 * Broken-down parsed date
	 */
	typedef struct {
		/// Parsed year value
		uint16_t year;
		/// Parsed month value
		uint8_t month;
		/// Parsed day value
		uint8_t day;
	} DateStruct;

	/**
	 * Broken-down parsed time
	 */
	typedef struct {
		/// Parsed hours value
		int8_t hours;
		/// Parsed minutes value
		int8_t minutes;
		/// Parsed seconds value
		int8_t seconds;
		/// Parsed milliseconds value
		int16_t milliseconds;
	} TimeStruct;

	/**
	 * Checks if passed timestamp is a valid ISO 8601 date-time string
	 * @param timestamp Timestamp string
	 * @return true if timestamp is valid, false otherwise
	 */
	static bool is_timestamp_valid(std::string &timestamp) {
		std::regex r("^\\d{4}-(0[1-9]|1[0-2])-(0[1-9]|[12]\\d|3[01])T([01]\\d|2[0-3]):([0-5]\\d):([0-5]\\d)(\\.\\d{3})?(Z|\\+(0\\d|1[0-3]):[0-5]\\d|\\+14:00|-(0\\d|1[0-1]):[0-5]\\d|-12:00)$", std::regex::icase);
		return std::regex_match(timestamp, r);
	}

	/**
	 * Parses date from timestamp string into a broken-down date struct
	 * @param sv Timestamp string
	 * @return const DateStruct Parsed broken-down date
	 */
	static const DateStruct parse_date(std::string_view &sv) {
		DateStruct date{0, 0, 0};
		date.year = remove_prefix_to_integer<uint16_t>(sv, 4);
		if (is_special_character(sv)) {
			sv.remove_prefix(1);
		}
		date.month = remove_prefix_to_integer<uint8_t>(sv, 2);
		if (is_special_character(sv)) {
			sv.remove_prefix(1);
		}
		date.day = remove_prefix_to_integer<uint8_t>(sv, 2);
		if (is_special_character(sv)) {
			sv.remove_prefix(1);
		}
		return date;
	}

	/**
	 * Parses time from timestmap string into a broken-down time struct
	 * @param sv Timestamp string
	 * @return const TimeStruct Parsed broken-down time
	 */
	static const TimeStruct parse_time(std::string_view &sv) {
		TimeStruct time{0, 0, 0, 0};
		time.hours = remove_prefix_to_integer<int8_t>(sv, 2);
		if (is_special_character(sv)) {
			sv.remove_prefix(1);
		}
		time.minutes = remove_prefix_to_integer<int8_t>(sv, 2);
		if (is_special_character(sv)) {
			sv.remove_prefix(1);
		}
		time.seconds = remove_prefix_to_integer<int8_t>(sv, 2);
		if (sv[0] == '.') {
			sv.remove_prefix(1);
			time.milliseconds = remove_prefix_to_integer<int16_t>(sv, 3);
			while (!is_special_character(sv)) {
				sv.remove_prefix(1);
			}
		}
		return time;
	}

	/**
	 * Parses offset from timestamp into minutes
	 * @param sv Timestamp string
	 * @return int16_t Parsed offset in minutes
	 */
	static int16_t parse_offset(std::string_view &sv) {
		if (sv.size() == 0 || std::toupper(sv[0]) == 'Z') {
			return 0;
		}
		int16_t sign = 1;
		if (sv[0] == '-') {
			sign = -1;
			sv.remove_prefix(1);
		} else if (sv[0] == '+') {
			sv.remove_prefix(1);
		}

		int16_t offset = remove_prefix_to_integer<int16_t>(sv, 2) * 60;
		if (is_special_character(sv)) {
			sv.remove_prefix(1);
		}
		offset += remove_prefix_to_integer<int16_t>(sv, 2);
		return static_cast<int16_t>(sign * offset);
	}

	/**
	 * Checks if the first character is a non-digit character
	 * @param sv Datetime string
	 * @return true if the first character is not a digit, false otherwise
	 */
	static bool is_special_character(std::string_view &sv) {
		return !sv.empty() && !std::isdigit(sv[0]);
	}

	/**
	 * Cast character to specified integer type
	 * @tparam IntegerType Integer type
	 * @param c Character to cast
	 * @return const IntegerType Converted value
	 */
	template<typename IntegerType>
	static const IntegerType cast_to_integer(const char c) {
		return static_cast<IntegerType>(c - '0');
	}

	/**
	 * Removes prefix of specified length from a string, and converts removed characters to an integer value
	 * @tparam IntegerType Integer type
	 * @param sv Datetime string
	 * @param digits Number of digits to remove as prefix and convert
	 * @return const IntegerType Converted value
	 */
	template<typename IntegerType>
	static const IntegerType remove_prefix_to_integer(std::string_view &sv, size_t digits) {
		if (digits == 0 || digits > sv.size()) {
			throw std::logic_error("Invalid number of digits to remove.");
		}
		IntegerType value = 0;
		for (size_t i = 0; i < digits; ++i) {
			value += cast_to_integer<IntegerType>(sv[i]);
			if (i < (digits - 1)) {
				value *= 10;
			}
		}
		sv.remove_prefix(digits);
		return value;
	}
};
