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

#include "ActionRecordParser.h"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace iqrf::metadata {

  TEST(ActionRecordParserTest, parse) {
    json doc({
      {"memory", {
        {"type", 5},
        {"addr", 10},
        {"size", 30}
      }},
      {"commands", {
        {
          {"value", 0},
          {"text", "Do nothing"}
        }
      }}
    });

    auto parsed = ActionRecordParser::parse(doc);
    ASSERT_TRUE(parsed.memory().has_value());
    ASSERT_TRUE(parsed.memory().value().type().has_value());
    EXPECT_EQ(5, parsed.memory().value().type().value());
    ASSERT_TRUE(parsed.memory().value().address().has_value());
    EXPECT_EQ(10, parsed.memory().value().address().value());
    ASSERT_TRUE(parsed.memory().value().size().has_value());
    EXPECT_EQ(30, parsed.memory().value().size().value());
    ASSERT_EQ(1, parsed.commands().size());
    ASSERT_TRUE(parsed.commands()[0].value().has_value());
    EXPECT_EQ(0, parsed.commands()[0].value());
    ASSERT_TRUE(parsed.commands()[0].text().has_value());
    EXPECT_EQ("Do nothing", parsed.commands()[0].text());
  }

}
