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

#include "ExternalParser.h"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace iqrf::metadata {

  TEST(ExternalParserTest, parse_specified) {
    json doc({
      {"type", "DC"},
      {"nominalVoltage", 5},
      {"minVoltage", 3.75},
      {"maxVoltage", 6}
    });

    auto parsed = ExternalParser::parse(doc);
    ASSERT_TRUE(parsed.type().has_value());
    EXPECT_EQ("DC", parsed.type().value());
    ASSERT_TRUE(parsed.nominalVoltage().has_value());
    EXPECT_EQ(5, parsed.nominalVoltage().value());
    ASSERT_TRUE(parsed.minVoltage().has_value());
    EXPECT_EQ(3.75, parsed.minVoltage().value());
    ASSERT_TRUE(parsed.maxVoltage().has_value());
    EXPECT_EQ(6, parsed.maxVoltage().value());
  }

}
