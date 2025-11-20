#pragma once

#include <boost/system/error_code.hpp>
#include <boost/beast/core/error.hpp>
#include <nlohmann/json.hpp>

enum class auth_error {
  success = 0,
  no_auth,
  auth_timeout,
  auth_failed,
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

inline std::string create_error_message(const boost::system::error_code& ec) {
  return nlohmann::json({
    {"type", "auth_error"},
    {"code", ec.value()},
    {"error", ec.message()},
  }).dump();
}
