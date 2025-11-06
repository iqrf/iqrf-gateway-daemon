#pragma once

#include <algorithm>
#include <chrono>
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
  static std::time_t get_current_timestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  }

  /**
   * @brief Parse an expiration time string into a Unix timestamp.
   *
   * Supports:
   *    relative format (e.g. "30d" = 30 days from now)
   *    Unix timestamp (e.g. "1700000000")
   * @param input Input time string
   * @param created_at Created at timestamp for computing expiration
   * @return Expiration Unix timestamp
   */
  static std::time_t parse_expiration(const std::string& input, std::time_t created_at) {
    // numerical, input as unix epoch timestamp
    if (!input.empty() && std::all_of(input.begin(), input.end(), ::isdigit)) {
      auto candidate = std::stoll(input);
      if (candidate <= created_at) {
        throw std::invalid_argument("Expiration unix timestamp is in the past.");
      }
      return candidate;
    }

    // relative
    auto unit = input.back();
    auto num_part = input.substr(0, input.length() - 1);
    if (std::all_of(num_part.begin(), num_part.end(), ::isdigit) && is_time_unit(unit)) {
      auto numerical = std::stoi(num_part);
      if (numerical < 1) {
        throw std::invalid_argument("Unit count should be a positive integer.");
      }
      auto value = std::chrono::hours(1);
      switch (unit) {
      case 'd':
        value *= 24;
        break;
      case 'w':
        value *= 24 * 7;
        break;
      case 'm':
        value *= 24 * 30;
        break;
      case 'y':
        value *= 24 * 365;
        break;
      default:
        break;
      }
      // mutliply unit by count
      value *= numerical;
      auto diff = std::chrono::duration_cast<std::chrono::seconds>(value).count();
      return created_at + diff;
    }
    throw std::invalid_argument("Invalid expiration time argument.");
  }
};
