/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
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

#include "BatteryParser.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

using json = nlohmann::json;

namespace iqrf::metadata {

  TEST(BatteryParserTest, parseV0_specified) {
    json doc({
      {"present", true},
      {"type", "CR2450"},
      {"changeThreshold", 2.4}
    });

    auto parsed = BatteryParser::parseV0(doc);

    EXPECT_TRUE(parsed.present());
    ASSERT_TRUE(parsed.type().has_value());
    EXPECT_EQ("CR2450", parsed.type().value());
    ASSERT_TRUE(parsed.changeThreshold().has_value());
    EXPECT_EQ(2.4, parsed.changeThreshold().value());
    EXPECT_FALSE(parsed.conditioning().has_value());
  }

  TEST(BatteryParserTest, parseV0_unspecified) {
    json doc({
      {"present", false},
      {"type", nullptr},
      {"changeThreshold", nullptr}
    });

    auto parsed = BatteryParser::parseV0(doc);

    EXPECT_FALSE(parsed.present());
    EXPECT_FALSE(parsed.type().has_value());
    EXPECT_FALSE(parsed.changeThreshold().has_value());
    EXPECT_FALSE(parsed.conditioning().has_value());
  }

  TEST(BatteryParserTest, parseV1_specified) {
    json doc({
      {"present", true},
      {"type", "CR2450"},
      {"changeThreshold", 2.4},
      {"conditioning", true}
    });

    auto parsed = BatteryParser::parseV1(doc);

    EXPECT_TRUE(parsed.present());
    ASSERT_TRUE(parsed.type().has_value());
    EXPECT_EQ("CR2450", parsed.type().value());
    ASSERT_TRUE(parsed.changeThreshold().has_value());
    EXPECT_EQ(2.4, parsed.changeThreshold().value());
    ASSERT_TRUE(parsed.conditioning().has_value());
    EXPECT_TRUE(parsed.conditioning().value());
  }

  TEST(BatteryParserTest, parseV1_unspecified) {
    json doc({
      {"present", false},
      {"type", nullptr},
      {"changeThreshold", nullptr}
    });

    auto parsed = BatteryParser::parseV1(doc);

    EXPECT_FALSE(parsed.present());
    EXPECT_FALSE(parsed.type().has_value());
    EXPECT_FALSE(parsed.changeThreshold().has_value());
    EXPECT_FALSE(parsed.conditioning().has_value());
  }

  class BatteryParserParameterizedErrorTest : public ::testing::TestWithParam<json> {};

  TEST_P(BatteryParserParameterizedErrorTest, parse_missing_fields) {
    auto doc = GetParam();
    EXPECT_THROW(BatteryParser::parseV0(doc), std::invalid_argument);
    EXPECT_THROW(BatteryParser::parseV1(doc), std::invalid_argument);
  }

  INSTANTIATE_TEST_CASE_P(
    BatteryParserTest,
    BatteryParserParameterizedErrorTest,
    ::testing::Values(
      json({
        {"type", nullptr},
        {"changeThreshold", nullptr}
      }),
      json({
        {"present", false},
        {"changeThreshold", nullptr}
      }),
      json({
        {"present", false},
        {"type", nullptr},
      })
    )
  );
}
