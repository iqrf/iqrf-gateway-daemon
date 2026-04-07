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

#include "PersistentQuantityParser.h"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace iqrf::metadata {

  TEST(PersistentQuantityParserTest, parse) {
    json doc({
      {"quantity", 1},
      {"value", 75},
      {"description", "Temperature test"},
      {"memory", {
        {"type", 5},
        {"addr", 10}
      }}
    });

    auto parsed = PersistentQuantityParser::parse(doc);
    ASSERT_TRUE(parsed.quantity().has_value());
    EXPECT_EQ(1, parsed.quantity().value());
    ASSERT_TRUE(parsed.value().has_value());
    EXPECT_EQ(75, parsed.value().value());
    ASSERT_TRUE(parsed.description().has_value());
    EXPECT_EQ("Temperature test", parsed.description().value());
    ASSERT_TRUE(parsed.quantityMemory().has_value());
    ASSERT_TRUE(parsed.quantityMemory().value().type().has_value());
    EXPECT_EQ(5, parsed.quantityMemory().value().type().value());
    ASSERT_TRUE(parsed.quantityMemory().value().address().has_value());
    EXPECT_EQ(10, parsed.quantityMemory().value().address().value());
  }

}
