#pragma once

#include "DatetimeParser.h"
#include <algorithm>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>

class DateTimeUtils {
public:
  /**
   * @brief Check if a character represents valid time unit:
   *        'd': days
   *        'w': weeks
   *        'm': months
   *        'y': years
   * @param c Character to check
   * @return True if character is a supported time unit, false otherwise.
   */
  static bool is_time_unit(const char c) {
    return c == 'd' || c == 'w' || c == 'm' || c == 'y';
  }

  /**
   * @brief Get current time as Unix epoch timestamp
   * @return Current time in seconds since epoch
   */
  static int64_t get_current_timestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  }

  /**
   * @brief Parse an expiration time string into a time point.
   *
   * Supports:
   *    relative format (e.g. "30d" = 30 days from now)
   *    ISO8601 datetime timestamp (e.g. "2024-05-05T10:20:30+01:00")
   * @param input Input time string
   * @param created_at Created at time point for computing expiration
   * @return `std::chrono::system_clock::time_point` Expiration time point
   */
  static std::chrono::system_clock::time_point parse_expiration(const std::string& input, const std::chrono::system_clock::time_point& created_at) {
    // numerical, input as unix epoch timestamp
    if (!input.empty()) {
      std::optional<std::chrono::system_clock::time_point> candidate;
      try {
        candidate.emplace(DatetimeParser::parse_to_timepoint(input));
      } catch (const std::invalid_argument &e) {
        // invalid string, catch to allow to attempt parsing relative
      }
      if (candidate.has_value()) {
        if (candidate.value() <= created_at) {
          throw std::invalid_argument("Expiration timestamp is in the past.");
        }
        return candidate.value();
      }
    }

    // relative
    auto unit = input.back();
    auto num_part = input.substr(0, input.length() - 1);
    if (std::all_of(num_part.begin(), num_part.end(), ::isdigit) && is_time_unit(unit)) {
      auto numerical = std::stoi(num_part);
      if (numerical < 1) {
        throw std::invalid_argument("Unit count should be a positive integer.");
      }
      auto offset = std::chrono::hours(1);
      switch (unit) {
      case 'd':
        offset *= 24;
        break;
      case 'w':
        offset *= 24 * 7;
        break;
      case 'm':
        offset *= 24 * 30;
        break;
      case 'y':
        offset *= 24 * 365;
        break;
      default:
        break;
      }
      // mutliply unit by count
      offset *= numerical;
      return created_at + offset;
    }
    throw std::invalid_argument("Invalid expiration time argument.");
  }
};
