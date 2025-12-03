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

#include <boost/system/error_code.hpp>
#include <boost/beast/core/error.hpp>
#include <nlohmann/json.hpp>

enum class auth_error {
  success = 0,
  no_auth,
  auth_timeout,
  auth_failed,
  token_not_found,
  invalid_token,
  expired_token,
  revoked_token,
};

class auth_error_category : public boost::system::error_category {
public:
  const char* name() const noexcept override {
    return "auth";
  }

  std::string message(int val) const override {
    switch (static_cast<auth_error>(val)) {
      case auth_error::success:
        return "Success";
      case auth_error::no_auth:
        return "Unauthenticated";
      case auth_error::auth_timeout:
        return "Authentication timeout";
      case auth_error::auth_failed:
        return "Authentication failed";
      case auth_error::token_not_found:
        return "Token not found";
      case auth_error::invalid_token:
        return "Invalid token";
      case auth_error::expired_token:
        return "Expired token";
      case auth_error::revoked_token:
        return "Revoked token";
      default: return "Unknown auth error";
    }
  }
};

inline const boost::system::error_category& auth_category() {
  static auth_error_category instance;
  return instance;
}

inline boost::system::error_code make_error_code(auth_error err) {
  return {static_cast<int>(err), auth_category()};
}

inline std::string create_auth_success_message(int64_t expiration) {
  return nlohmann::json({
    {"type", "auth_success"},
    {"expiration", expiration},
  }).dump();
}

inline std::string create_auth_error_message(const boost::system::error_code& ec) {
  return nlohmann::json({
    {"type", "auth_error"},
    {"code", ec.value()},
    {"error", ec.message()},
  }).dump();
}
