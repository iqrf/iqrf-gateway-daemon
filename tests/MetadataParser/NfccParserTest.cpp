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

#include "NfccParser.h"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace iqrf::metadata {

  TEST(NfccParserTest, parse) {
    json doc({
      {"version", 1}
    });

    auto parsed = NfccParser::parse(doc);
    ASSERT_TRUE(parsed.version().has_value());
    EXPECT_EQ(1, parsed.version().value());
  }

}
