#pragma once

#include <date/date.h>
#include <string_view>

class DatetimeParser {
public:
	static const std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> parse_to_timepoint(std::string_view sv) {
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
	struct DateStruct {
		uint16_t year;
		uint8_t month;
		uint8_t day;
	};

	struct TimeStruct {
		int8_t hours;
		int8_t minutes;
		int8_t seconds;
		int16_t milliseconds;
	};

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

	static const int16_t parse_offset(std::string_view &sv) {
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

	static bool is_special_character(std::string_view &sv) {
		return !sv.empty() && !std::isdigit(sv[0]);
	}

	template<typename IntegerType>
	static const IntegerType cast_to_integer(const char c) {
		return static_cast<IntegerType>(c - '0');
	}

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
