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

#include <algorithm>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <vector>

#pragma once

#define ENCODED_SECRET_LEN 44

namespace iqrf {

  class ApiTokenUtils {
   public:
    /**
    * @brief Check if character is a valid base64 character
    * @param c Character to check
    * @return True if character is a valid base64 character, false otherwise
    */
    static bool isBase64Character(const char c) {
      return (std::isalnum(static_cast<unsigned char>(c)) || c == '+' || c == '/' || c == '=');
    }

    /**
    * @brief Parse token into ID and secret
    * @param token API token
    * @param id Token ID container
    * @param secret Secret container
    */
    static void parseToken(const std::string& token, uint32_t& id, std::string& secret) {
      const char delim = ';';
      size_t start = 0;
      size_t end = token.find(delim);

      std::vector<std::string> parts;

      while (end != std::string::npos) {
        parts.emplace_back(token.substr(start, end - start));
        start = end + 1;
        end = token.find(delim, start);
      }
      parts.emplace_back(token.substr(start));

      if (parts.size() != 3) {
        throw std::invalid_argument("Invalid API token format.");
      }

      if (parts[0] != "iqrfgd2") {
        throw std::invalid_argument("Unsupported API token type.");
      }

      if (!std::all_of(parts[1].begin(), parts[1].end(), ::isdigit)) {
        throw std::invalid_argument("API token ID part contains non-numeric characters.");
      }
      id = static_cast<uint32_t>(std::stoul(parts[1]));
      if (parts[2].length() != ENCODED_SECRET_LEN) {
        throw std::invalid_argument("Invalid API token secret length.");
      }
      if (!std::all_of(parts[2].begin(), parts[2].end(), isBase64Character)) {
        throw std::invalid_argument("API token secret contains invalid characters.");
      }
      secret = parts[2];
    }
  };

}  // iqrf namespace
