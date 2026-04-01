#include <gtest/gtest.h>
#include "WebsocketServerUtils.h"

TEST(WebsocketServerUtilsTest, is_auth_message_ok) {
  nlohmann::json doc({
    {"type", "auth"},
    {"token", "iqrfgd2;1;zDrcvQaXWopzJ+DbfkpGq3Tn00wkt3n6fExj8iUsYio="}
  });
  EXPECT_TRUE(is_auth_message(doc));
}

TEST(WebsocketServerUtilsTest, is_auth_message_invalid) {
  nlohmann::json doc({
    {"nonsense", "not_auth"}
  });
  EXPECT_FALSE(is_auth_message(doc));
}

TEST(WebsocketServerUtilsTest, create_auth_success_message_ok) {
  std::string expected = R"({"expiration":"2025-12-03T10:16:55Z","service":true,"type":"auth_success"})";
  EXPECT_EQ(expected, create_auth_success_message("2025-12-03T10:16:55Z", true));
}

TEST(WebsocketServerUtilsTest, create_auth_error_message_ok) {
  std::string expected = R"({"code":3,"error":"Authentication failed","type":"auth_error"})";
  EXPECT_EQ(expected, create_auth_error_message(make_error_code(auth_error::auth_failed)));
}
