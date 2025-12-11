/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#include <gtest/gtest.h>
#include "TimeConversion.h"

namespace time_conversion_test {

class TimeConversionTest : public ::testing::Test {
protected:
  std::chrono::system_clock::time_point tpSec{std::chrono::seconds(1765448430)};
  std::chrono::system_clock::time_point tpMillis{std::chrono::milliseconds(1765448430337)};
};

TEST_F(TimeConversionTest, get_epoch_timestamp) {
  EXPECT_EQ(1765448430, TimeConversion::getEpochTimestamp(tpSec));
}

TEST_F(TimeConversionTest, get_iso8601_timestamp_safe_sec) {
  EXPECT_EQ("2025-12-11T10:20:30Z", TimeConversion::getISO8601TimestampSafe(tpSec, false));
}

TEST_F(TimeConversionTest, get_iso8601_timestamp_safe_millis) {
  EXPECT_EQ("2025-12-11T10:20:30.337Z", TimeConversion::getISO8601TimestampSafe(tpMillis));
}

}
