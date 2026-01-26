#pragma once

#include <optional>

namespace test_utils {

  struct Result {
    bool status;
    std::optional<std::string_view> error;
  };

} // namespace test_utils
