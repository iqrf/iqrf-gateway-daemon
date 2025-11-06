#include <gtest/gtest.h>
#include "cli_utils.h"

#include <boost/program_options.hpp>

namespace bpo = boost::program_options;
using json = nlohmann::json;

class CliUtilsTest : public ::testing::Test {
protected:
  uint32_t id = 1;
  std::string owner = "test";
  std::string salt = "G51ubXO8K+NIBjms7zRarw==";
  std::string hash = "JzDeC2H1D8+rG0/Z9QABWz3APZ++lfmC2tzwTOgaIC8=";
  int64_t created_at = 1730726400;
  int64_t expires_at = 1731590400;
  bool revoked = false;
  bool service = false;

  iqrf::db::models::ApiToken token;

  void SetUp() {
    token = iqrf::db::models::ApiToken(
      id,
      owner,
      salt,
      hash,
      created_at,
      expires_at,
      revoked,
      service
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

TEST_F(CliUtilsTest, token_to_json_value) {
  auto doc = token_to_json(token);
  EXPECT_TRUE(doc.count("id"));
  EXPECT_EQ(id, doc["id"]);
  EXPECT_TRUE(doc.count("owner"));
  EXPECT_EQ(owner, doc["owner"]);
  EXPECT_TRUE(doc.count("created_at"));
  EXPECT_EQ(created_at, doc["created_at"]);
  EXPECT_TRUE(doc.count("expires_at"));
  EXPECT_EQ(expires_at, doc["expires_at"]);
  EXPECT_TRUE(doc.count("revoked"));
  EXPECT_FALSE(doc["revoked"]);
  EXPECT_TRUE(doc.count("service"));
  EXPECT_FALSE(doc["service"]);
}

TEST_F(CliUtilsTest, token_to_json_string_value) {
  std::string expected = "{\"created_at\":1730726400,\"expires_at\":1731590400,\"id\":1,\"owner\":\"test\",\"revoked\":false,\"service\":false}";
  EXPECT_EQ(expected, token_to_json_string(token));
}
