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

#include "AccumulatorParser.h"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

namespace iqrf::metadata {

  TEST(AccumulatorParserTest, parse_specified) {
    json doc({
      {"present", true},
      {"type", "LIP552240"},
      {"lowLevel", 3.4}
    });

    auto parsed = AccumulatorParser::parse(doc);

    EXPECT_TRUE(parsed.present());
    ASSERT_TRUE(parsed.type().has_value());
    EXPECT_EQ("LIP552240", parsed.type().value());
    ASSERT_TRUE(parsed.lowLevel().has_value());
    EXPECT_EQ(3.4, parsed.lowLevel().value());
  }

  TEST(AccumulatorParserTest, parse_unspecified) {
    json doc({
      {"present", false},
      {"type", nullptr},
      {"lowLevel", nullptr}
    });

    auto parsed = AccumulatorParser::parse(doc);

    EXPECT_FALSE(parsed.present());
    EXPECT_FALSE(parsed.type().has_value());
    EXPECT_FALSE(parsed.lowLevel().has_value());
  }

  class AccumulatorParserParameterizedErrorTest : public ::testing::TestWithParam<json> {};

  TEST_P(AccumulatorParserParameterizedErrorTest, parse_missing_fields) {
    auto doc = GetParam();
    EXPECT_THROW(AccumulatorParser::parse(doc), std::invalid_argument);
  }

  INSTANTIATE_TEST_CASE_P(
    AccumulatorParserTest,
    AccumulatorParserParameterizedErrorTest,
    ::testing::Values(
      json({
        {"type", nullptr},
        {"lowLevel", nullptr}
      }),
      json({
        {"present", false},
        {"lowLevel", nullptr}
      }),
      json({
        {"present", false},
        {"type", nullptr},
      })
    )
  );
}
