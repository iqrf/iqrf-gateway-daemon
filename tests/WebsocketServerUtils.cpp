#include <gtest/gtest.h>
#include "WebsocketServerUtils.h"

TEST(WebsocketServerUtilsTest, create_simple_error_message) {
  std::string expected = R"({"error":"Authentication failed."})";
  EXPECT_EQ(expected, create_error_message("Authentication failed."));
}
