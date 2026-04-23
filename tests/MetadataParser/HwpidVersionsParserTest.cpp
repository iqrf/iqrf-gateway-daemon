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

#include "HwpidVersionsParser.h"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace iqrf::metadata {

  TEST(HwpidVersionsParserTest, parse_specified) {
    json doc({
      {"min", 0},
      {"max", 5}
    });

    auto parsed = HwpidVersionsParser::parse(doc);
    EXPECT_EQ(0, parsed.min());
    ASSERT_TRUE(parsed.max().has_value());
    EXPECT_EQ(5, parsed.max().value());
  }

  TEST(HwpidVersionsParserTest, parse_max_unspecified) {
    json doc({
      {"min", 0},
      {"max", -1}
    });

    auto parsed = HwpidVersionsParser::parse(doc);
    EXPECT_EQ(0, parsed.min());
    EXPECT_FALSE(parsed.max().has_value());
  }

  class HwpidVersionsParserParameterizedErrorTest : public ::testing::TestWithParam<json> {};

  TEST_P(HwpidVersionsParserParameterizedErrorTest, parse_missing_fields) {
    auto doc = GetParam();
    EXPECT_THROW(HwpidVersionsParser::parse(doc), std::invalid_argument);
  }

  INSTANTIATE_TEST_CASE_P(
    HwpidVersionsParserTest,
    HwpidVersionsParserParameterizedErrorTest,
    ::testing::Values(
      json({
        {"min", 0}
      }),
      json({
        {"max", -1}
      })
    )
  );

}
