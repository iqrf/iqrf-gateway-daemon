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

#include <string>
#include <vector>

namespace iqrf {
	static const char *STRING_UTILS_WHITESPACE = " \t\n\r\f\v";
	class StringUtils {
	public:

		/**
		 * Trim from start
		 * @param s String to trim
		 * @return Trimmed string
		 */
		static void ltrim(std::string &s) {
			s.erase(0, s.find_first_not_of(STRING_UTILS_WHITESPACE));
		}

		/**
		 * Trim from end
		 * @param s String to trim
		 * @return Trimmed string
		 */
		static void rtrim(std::string &s) {
			s.erase(s.find_last_not_of(STRING_UTILS_WHITESPACE) + 1);
		}

		/**
		 * Trim from both ends
		 * @param s String to trim
		 * @return Trimmed string
		 */
		static void trim(std::string &s) {
			rtrim(s);
			ltrim(s);
		}

		/**
		 * Check if string starts with prefix
		 * @param s String to check
		 * @param prefix Prefix to check
		 * @return True if string starts with prefix
		 */
		static bool startsWith(const std::string &s, const std::string &prefix) {
			return s.rfind(prefix, 0) == 0;
		}

		/**
		 * Check if string ends with suffix
		 * @param s String to check
		 * @param suffix Suffix to check
		 * @return True if string ends with suffix
		 */
		static bool endsWith(const std::string &s, const std::string &suffix) {
			auto res = s.rfind(suffix);
			return (res != std::string::npos) && res == (s.size() - suffix.size());
		}

		/**
		 * Split string by delimiter
		 * @param string String to split
		 * @param delimiter Delimiter
		 * @return Vector of strings
		 */
		static std::vector<std::string> split(const std::string &string, const std::string &delimiter) {
			std::string token;
			std::vector<std::string> tokens;
			size_t needle, start = 0, len = delimiter.length();
			while ((needle = string.find(delimiter, start)) != std::string::npos) {
				token = string.substr(start, needle - start);
				tokens.push_back(token);
				start = needle + len;
			}
			tokens.push_back(string.substr(start));
			return tokens;
		}
	};
}
