#include <gtest/gtest.h>
#include "WebsocketServerUtils.h"

TEST(WebsocketServerUtilsTest, create_auth_success_message_ok) {
  std::string expected = R"({"expiration":1764756999,"type":"auth_success"})";
  EXPECT_EQ(expected, create_auth_success_message(1764756999));
}

TEST(WebsocketServerUtilsTest, create_auth_error_message_ok) {
  std::string expected = R"({"code":3,"error":"Authentication failed","type":"auth_error"})";
  EXPECT_EQ(expected, create_auth_error_message(make_error_code(auth_error::auth_failed)));
}
