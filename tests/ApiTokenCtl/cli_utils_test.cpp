#include <chrono>
#include <gtest/gtest.h>
#include "cli_utils.h"
#include "date/date.h"

#include <boost/program_options.hpp>
#include <optional>

namespace bpo = boost::program_options;
using json = nlohmann::json;

class CliUtilsTest : public ::testing::Test {
protected:
  uint32_t id = 1;
  std::string owner = "test";
  std::string salt = "G51ubXO8K+NIBjms7zRarw==";
  std::string hash = "JzDeC2H1D8+rG0/Z9QABWz3APZ++lfmC2tzwTOgaIC8=";
  std::string created_at_str = "2024-11-04T01:20:00Z";
  std::chrono::system_clock::time_point created_at =
    date::sys_days{date::year(2024)/11/4} +
    std::chrono::hours(1) +
    std::chrono::minutes(20);
  std::string expires_at_str = "2024-11-14T01:20:00Z";
  std::chrono::system_clock::time_point expires_at =
    date::sys_days{date::year(2024)/11/14} +
    std::chrono::hours(1) +
    std::chrono::minutes(20);
  iqrf::db::models::ApiToken::Status status = iqrf::db::models::ApiToken::Status::Valid;
  bool service = false;
  std::optional<std::chrono::system_clock::time_point> invalidated_at = std::nullopt;

  iqrf::db::models::ApiToken token;

  void SetUp() {
    token = iqrf::db::models::ApiToken(
      id,
      owner,
      salt,
      hash,
      created_at,
      expires_at,
      status,
      service,
      invalidated_at
    );
  }
};

TEST(cli_utils, make_shared_params_valid) {
  bpo::variables_map vm;
  const std::string path = "/path/to/sqlite.db";
  vm.insert(std::make_pair("path", bpo::variable_value(boost::any(path), false)));
  vm.insert(std::make_pair("json", bpo::variable_value(boost::any(true), false)));

  auto params = make_shared_params(vm);
  EXPECT_EQ(path, params.db_path);
  EXPECT_EQ(true, params.json_output);
}

TEST(cli_utils, get_token_id_valid) {
  bpo::variables_map vm;
  vm.insert(std::make_pair("id", bpo::variable_value(boost::int64_t(1), false)));

  EXPECT_EQ(1, get_token_id(vm));
}

TEST(cli_utils, get_token_id_invalid_range) {
  std::vector<int64_t> cases = {0, -10, 4294967300};
  std::string expected_error = "Token ID should be between 1 and 4294967295.";
  bpo::variables_map vm;

  for (auto &value : cases) {
    vm.insert(std::make_pair("id", bpo::variable_value(boost::int64_t(value), false)));
    try {
      get_token_id(vm);
      FAIL() << "Expected std::invalid_argument, but no exception was thrown.";
    } catch (const std::invalid_argument &e) {
      EXPECT_EQ(expected_error, e.what());
    } catch (...) {
      FAIL() << "Expected std::invalid_argument, but another exception was thrown instead.";
    }
  }
}

TEST_F(CliUtilsTest, token_to_json_valid) {
  auto now = token.getExpiresAt() - std::chrono::seconds(60);
  auto doc = token_to_json(token, now);
  EXPECT_TRUE(doc.count("id"));
  EXPECT_EQ(id, doc["id"]);
  EXPECT_TRUE(doc.count("owner"));
  EXPECT_EQ(owner, doc["owner"]);
  EXPECT_TRUE(doc.count("created_at"));
  EXPECT_EQ(created_at_str, doc["created_at"]);
  EXPECT_TRUE(doc.count("expires_at"));
  EXPECT_EQ(expires_at_str, doc["expires_at"]);
  EXPECT_TRUE(doc.count("status"));
  EXPECT_EQ(static_cast<int>(status), doc["status"]);
  EXPECT_TRUE(doc.count("service"));
  EXPECT_FALSE(doc["service"]);
  EXPECT_TRUE(doc.count("invalidated_at"));
  EXPECT_EQ(nullptr, doc["invalidated_at"]);
}

TEST_F(CliUtilsTest, token_to_json_marked_valid_expired) {
  auto now = token.getExpiresAt() + std::chrono::seconds(60);
  auto doc = token_to_json(token, now);
  EXPECT_TRUE(doc.count("id"));
  EXPECT_EQ(id, doc["id"]);
  EXPECT_TRUE(doc.count("owner"));
  EXPECT_EQ(owner, doc["owner"]);
  EXPECT_TRUE(doc.count("created_at"));
  EXPECT_EQ(created_at_str, doc["created_at"]);
  EXPECT_TRUE(doc.count("expires_at"));
  EXPECT_EQ(expires_at_str, doc["expires_at"]);
  EXPECT_TRUE(doc.count("status"));
  EXPECT_EQ(static_cast<int>(iqrf::db::models::ApiToken::Status::Expired), doc["status"]);
  EXPECT_TRUE(doc.count("service"));
  EXPECT_FALSE(doc["service"]);
  EXPECT_TRUE(doc.count("invalidated_at"));
  EXPECT_TRUE(doc["invalidated_at"].is_null());
}

TEST_F(CliUtilsTest, token_to_json_revoked) {
  std::chrono::system_clock::time_point revoked_at =
    date::sys_days{date::year{2024}/11/5} +
    std::chrono::hours{16} +
    std::chrono::minutes{24} +
    std::chrono::seconds{55};
  token.revoke(revoked_at);
  auto now = token.getExpiresAt() + std::chrono::seconds(60);
  auto doc = token_to_json(token, now);
  EXPECT_TRUE(doc.count("id"));
  EXPECT_EQ(id, doc["id"]);
  EXPECT_TRUE(doc.count("owner"));
  EXPECT_EQ(owner, doc["owner"]);
  EXPECT_TRUE(doc.count("created_at"));
  EXPECT_EQ(created_at_str, doc["created_at"]);
  EXPECT_TRUE(doc.count("expires_at"));
  EXPECT_EQ(expires_at_str, doc["expires_at"]);
  EXPECT_TRUE(doc.count("status"));
  EXPECT_EQ(static_cast<int>(iqrf::db::models::ApiToken::Status::Revoked), doc["status"]);
  EXPECT_TRUE(doc.count("service"));
  EXPECT_FALSE(doc["service"]);
  EXPECT_TRUE(doc.count("invalidated_at"));
  EXPECT_EQ("2024-11-05T16:24:55Z", doc["invalidated_at"]);
}

TEST_F(CliUtilsTest, token_to_json_string_valid) {
  auto now = token.getExpiresAt() - std::chrono::seconds(60);
  std::string expected = R"({"created_at":"2024-11-04T01:20:00Z","expires_at":"2024-11-14T01:20:00Z","id":1,"invalidated_at":null,"owner":"test","service":false,"status":0})";
  EXPECT_EQ(expected, token_to_json_string(token, now));
}

TEST_F(CliUtilsTest, token_to_json_string_marked_valid_expired) {
  auto now = token.getExpiresAt() + std::chrono::seconds(60);
  std::string expected = R"({"created_at":"2024-11-04T01:20:00Z","expires_at":"2024-11-14T01:20:00Z","id":1,"invalidated_at":null,"owner":"test","service":false,"status":1})";
  EXPECT_EQ(expected, token_to_json_string(token, now));
}

TEST_F(CliUtilsTest, token_to_json_string_revoked) {
  std::chrono::system_clock::time_point revoked_at =
    date::sys_days{date::year{2024}/11/5} +
    std::chrono::hours{16} +
    std::chrono::minutes{24} +
    std::chrono::seconds{55};
  token.revoke(revoked_at);
  auto now = token.getExpiresAt() + std::chrono::seconds(60);
  std::string expected = R"({"created_at":"2024-11-04T01:20:00Z","expires_at":"2024-11-14T01:20:00Z","id":1,"invalidated_at":"2024-11-05T16:24:55Z","owner":"test","service":false,"status":2})";
  EXPECT_EQ(expected, token_to_json_string(token, now));
}

TEST(cli_utils, pad_end_shorter) {
  EXPECT_EQ("test    ", pad_end("test", 8));
  EXPECT_EQ("test----", pad_end("test", 8, '-'));
  EXPECT_EQ("  test  ", pad_end("  test", 8));
}

TEST(cli_utils, pad_end_same_length) {
  EXPECT_EQ("test", pad_end("test", 4));
  EXPECT_EQ("test", pad_end("test", 4, '-'));
}

TEST(cli_utils, pad_end_longer) {
  EXPECT_EQ("teststri", pad_end("teststring", 8));
  EXPECT_EQ("testst", pad_end("teststring", 6, '-'));
}

TEST(cli_utils, construct_shareable_token_plain) {
  EXPECT_EQ(
    "iqrfgd2;1;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=",
    construct_shareable_token(1, "updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", false)
  );
}

TEST(cli_utils, construct_shareable_token_json) {
  EXPECT_EQ(
    "{\"id\":1,\"token\":\"iqrfgd2;1;updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=\"}",
    construct_shareable_token(1, "updgKmdhceGzmI9i0M9UELExYZwSKrCBv1vusq+TfHk=", true)
  );
}
