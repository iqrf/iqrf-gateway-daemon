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

#include <cpr/cpr.h>

#include <cerrno>
#include <cstring>
#include <string>

namespace iqrf {

    class CurlUtils {
    public:

        static void downloadFile(const std::string& url, const std::string& filePath, std::map<std::string, std::string> headers = {}) {
            cpr::Response rsp = cpr::Get(
                cpr::Url(url),
                headers
            );
            if (rsp.status_code != 200) {
                throw std::runtime_error("Unable to download file " + url + ":" + rsp.error.message);
            }
            try {
                createDirectory(filePath);
            } catch (const std::filesystem::filesystem_error& e) {
                throw std::runtime_error("Unable to create parent directory for file: " + std::string(e.what()));
            }
            std::ofstream file(filePath);
            if (!file.is_open()) {
                throw std::runtime_error("Unable to create file " + filePath + ":" + std::strerror(errno));
            }
            file << rsp.text;
            file.close();
        }
    private:

        static void createDirectory(const std::string& path) {
            if (std::filesystem::exists(path)) {
                return;
            }
            auto parentDir = std::filesystem::path(path).parent_path();
            if (!std::filesystem::exists(parentDir)) {
                std::filesystem::create_directories(parentDir);
            }
        }
    };

} // iqrf namespace
