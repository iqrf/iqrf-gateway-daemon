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

#include "BaseParser.h"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
#include <optional>

using json = nlohmann::json;

namespace iqrf::metadata {

  class BaseParserTest : public ::testing::Test, protected BaseParser {};

  TEST_F(BaseParserTest, parseValue) {
    json doc({
      {"test", 1},
      {"key", false},
      {"empty", nullptr}
    });

    EXPECT_EQ(1, BaseParser::parseValue<int>(doc, "test").value());
    EXPECT_FALSE(BaseParser::parseValue<bool>(doc, "key").value());
    EXPECT_FALSE(BaseParser::parseValue<std::string>(doc, "nonexistent").has_value());
    EXPECT_FALSE(BaseParser::parseValue<unsigned>(doc, "empty").has_value());
  }

}
