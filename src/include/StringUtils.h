/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

#include <string>
#include <vector>

namespace iqrf {
	static const char *STRING_UTILS_WHITESPACE = " \t\n\r\f\v";
	class StringUtils {
	public:
		static void ltrim(std::string &s) {
			s.erase(0, s.find_first_not_of(STRING_UTILS_WHITESPACE));
		}

		static void rtrim(std::string &s) {
			s.erase(s.find_last_not_of(STRING_UTILS_WHITESPACE) + 1);
		}

		static void trim(std::string &s) {
			rtrim(s);
			ltrim(s);
		}

		static bool starsWith(const std::string &s, const std::string &prefix) {
			return s.rfind(prefix, 0) == 0;
		}

		static bool endsWith(const std::string &s, const std::string &suffix) {
			auto res = s.rfind(suffix);
			return (res != std::string::npos) && res == (s.size() - suffix.size());
		}

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
