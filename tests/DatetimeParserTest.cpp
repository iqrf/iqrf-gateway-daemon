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
#include "DatetimeParser.h"

#include <chrono>
#include <ctime>
#include <string>

namespace datetime_parser_test {

class DatetimeParserTest : public ::testing::Test{
protected:

  static std::tm get_tm(const std::chrono::system_clock::time_point& tp) {
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    return *std::gmtime(&tt);
  }

  static int get_millis(const std::chrono::system_clock::time_point& tp) {
    auto dur = tp.time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    return static_cast<int>(ms % 1000);
  }
};

TEST_F(DatetimeParserTest, valid_utc) {
  auto tp = DatetimeParser::parse_to_timepoint("2025-12-14T01:25:40Z");
  std::tm tm = get_tm(tp);

  EXPECT_EQ(tm.tm_year + 1900, 2025);
  EXPECT_EQ(tm.tm_mon + 1, 12);
  EXPECT_EQ(tm.tm_mday, 14);
  EXPECT_EQ(tm.tm_hour, 1);
  EXPECT_EQ(tm.tm_min, 25);
  EXPECT_EQ(tm.tm_sec, 40);
  EXPECT_EQ(get_millis(tp), 0);
}

TEST_F(DatetimeParserTest, positive_offset) {
  auto tp = DatetimeParser::parse_to_timepoint("2025-12-14T12:00:00+02:00");
  // SHOULD BE 10 AM UTC
  std::tm tm = get_tm(tp);

  EXPECT_EQ(tm.tm_year + 1900, 2025);
  EXPECT_EQ(tm.tm_mon + 1, 12);
  EXPECT_EQ(tm.tm_mday, 14);
  EXPECT_EQ(tm.tm_hour, 10);
  EXPECT_EQ(tm.tm_min, 0);
  EXPECT_EQ(tm.tm_sec, 0);
  EXPECT_EQ(get_millis(tp), 0);
}

TEST_F(DatetimeParserTest, positive_offset_last_day) {
  auto tp = DatetimeParser::parse_to_timepoint("2025-12-14T02:00:00+05:00");
  // SHOULD BE 13th, 21 PM UTC
  std::tm tm = get_tm(tp);

  EXPECT_EQ(tm.tm_year + 1900, 2025);
  EXPECT_EQ(tm.tm_mon + 1, 12);
  EXPECT_EQ(tm.tm_mday, 13);
  EXPECT_EQ(tm.tm_hour, 21);
  EXPECT_EQ(tm.tm_min, 0);
  EXPECT_EQ(tm.tm_sec, 0);
  EXPECT_EQ(get_millis(tp), 0);
}

TEST_F(DatetimeParserTest, negative_offset) {
  auto tp = DatetimeParser::parse_to_timepoint("2025-12-14T12:00:00-05:00");
  // SHOULD BE 17 PM UTC
  std::tm tm = get_tm(tp);

  EXPECT_EQ(tm.tm_year + 1900, 2025);
  EXPECT_EQ(tm.tm_mon + 1, 12);
  EXPECT_EQ(tm.tm_mday, 14);
  EXPECT_EQ(tm.tm_hour, 17);
  EXPECT_EQ(tm.tm_min, 0);
  EXPECT_EQ(tm.tm_sec, 0);
  EXPECT_EQ(get_millis(tp), 0);
}

TEST_F(DatetimeParserTest, negative_offset_next_day) {
  auto tp = DatetimeParser::parse_to_timepoint("2025-12-14T23:00:00-05:00");
  // SHOULD BE 15th, 4 AM UTC
  std::tm tm = get_tm(tp);

  EXPECT_EQ(tm.tm_year + 1900, 2025);
  EXPECT_EQ(tm.tm_mon + 1, 12);
  EXPECT_EQ(tm.tm_mday, 15);
  EXPECT_EQ(tm.tm_hour, 4);
  EXPECT_EQ(tm.tm_min, 0);
  EXPECT_EQ(tm.tm_sec, 0);
  EXPECT_EQ(get_millis(tp), 0);
}

TEST_F(DatetimeParserTest, valid_millis) {
  auto tp = DatetimeParser::parse_to_timepoint("2025-12-14T12:00:00.789Z");
  std::tm tm = get_tm(tp);

  EXPECT_EQ(tm.tm_hour, 12);
  EXPECT_EQ(tm.tm_min, 0);
  EXPECT_EQ(tm.tm_sec, 0);
  EXPECT_EQ(get_millis(tp), 789);
}

TEST_F(DatetimeParserTest, valid_leap_year) {
  auto tp = DatetimeParser::parse_to_timepoint("2024-02-29T00:00:00Z");
  std::tm tm = get_tm(tp);

  EXPECT_EQ(tm.tm_year + 1900, 2024);
  EXPECT_EQ(tm.tm_mon + 1, 2);
  EXPECT_EQ(tm.tm_mday, 29);
  EXPECT_EQ(tm.tm_hour, 0);
  EXPECT_EQ(tm.tm_min, 0);
  EXPECT_EQ(tm.tm_sec, 0);
  EXPECT_EQ(get_millis(tp), 0);
}

TEST_F(DatetimeParserTest, non_leap_year_feb) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-02-29T00:00:00Z"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, invalid_month) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-13-29T00:00:00Z"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, invalid_day_30_month) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-04-31T00:00:00Z"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, invalid_day_31_month) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-01-32T00:00:00Z"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, invalid_hour) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-04-30T30:00:00Z"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, invalid_hour_edge) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-04-30T24:00:00Z"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, invalid_minute) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-04-30T10:78:00Z"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, invalid_minute_edge) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-04-30T10:60:00Z"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, invalid_second) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-04-30T23:00:61Z"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, missing_time) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-04-30"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, missing_time_T) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-04-30T"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, missing_time_with_zone) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-04-30Z"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, missing_date_T) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("T23:00:00Z"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, missing_date) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("23:00:00Z"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, missing_time_zone) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-04-30T23:00:00"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, missing_seconds) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-04-30T23:00Z"), std::invalid_argument);
}

TEST_F(DatetimeParserTest, lowercase_zone) {
  EXPECT_THROW(DatetimeParser::parse_to_timepoint("2021-04-30T23:00:00z"), std::invalid_argument);
}

}
