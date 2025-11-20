#include <gtest/gtest.h>
#include "WebsocketServerUtils.h"

TEST(WebsocketServerUtilsTest, create_simple_error_message) {
  std::string expected = R"({"code":3,"error":"Authentication failed","type":"auth_error"})";
  EXPECT_EQ(expected, create_error_message(make_error_code(auth_error::auth_failed)));
}
