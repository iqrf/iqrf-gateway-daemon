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

#include <chrono>
#include <date/date.h>
#include <gtest/gtest.h>
#include "DateTimeUtils.h"

TEST(DateTimeUtilsTest, is_time_unit_valid) {
  EXPECT_TRUE(DateTimeUtils::is_time_unit('d'));
  EXPECT_TRUE(DateTimeUtils::is_time_unit('w'));
  EXPECT_TRUE(DateTimeUtils::is_time_unit('m'));
  EXPECT_TRUE(DateTimeUtils::is_time_unit('y'));
}

TEST(DateTimeUtilsTest, is_time_unit_invalid) {
  EXPECT_FALSE(DateTimeUtils::is_time_unit('h'));
  EXPECT_FALSE(DateTimeUtils::is_time_unit('s'));
  EXPECT_FALSE(DateTimeUtils::is_time_unit('*'));
  EXPECT_FALSE(DateTimeUtils::is_time_unit('D'));
  EXPECT_FALSE(DateTimeUtils::is_time_unit('W'));
  EXPECT_FALSE(DateTimeUtils::is_time_unit('M'));
  EXPECT_FALSE(DateTimeUtils::is_time_unit('Y'));
}

TEST(DateTimeUtilsTest, parse_expiration_timestamp_valid) {
  std::chrono::system_clock::time_point tp =
    date::sys_days{date::year{2025}/11/4} +
    std::chrono::hours(6) +
    std::chrono::minutes(36) +
    std::chrono::seconds(55);
  std::chrono::system_clock::time_point expiration =
    date::sys_days{date::year{2025}/11/4} +
    std::chrono::hours(6) +
    std::chrono::minutes(38) +
    std::chrono::seconds(21);
  EXPECT_EQ(expiration, DateTimeUtils::parse_expiration("2025-11-04T06:38:21Z", tp));
  EXPECT_EQ(expiration, DateTimeUtils::parse_expiration("2025-11-04T06:38:21+00:00", tp));
  EXPECT_EQ(expiration, DateTimeUtils::parse_expiration("2025-11-04T07:38:21+01:00", tp));
  EXPECT_EQ(expiration, DateTimeUtils::parse_expiration("2025-11-04T11:38:21+05:00", tp));
  EXPECT_EQ(expiration, DateTimeUtils::parse_expiration("2025-11-04T08:08:21+01:30", tp));
}

TEST(DateTimeUtilsTest, parse_expiration_timestamp_past) {
  std::chrono::system_clock::time_point now =
    date::sys_days{date::year{2025}/11/4} +
    std::chrono::hours(6) +
    std::chrono::minutes(36) +
    std::chrono::seconds(55);
  try {
    DateTimeUtils::parse_expiration("2025-11-04T07:36:55+01:00", now);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Expiration timestamp is in the past.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
}

TEST(DateTimeUtilsTest, parse_expiration_relative_valid) {
  auto now = std::chrono::system_clock::now();
  auto expiration = now + std::chrono::hours(24 * 10);
  EXPECT_EQ(expiration, DateTimeUtils::parse_expiration("10d", now));
  expiration = now + std::chrono::hours(24 * 7);
  EXPECT_EQ(expiration, DateTimeUtils::parse_expiration("1w", now));
  expiration = now + std::chrono::hours(24 * 90);
  EXPECT_EQ(expiration, DateTimeUtils::parse_expiration("3m", now));
  expiration = now + std::chrono::hours(24 * 365 * 2);
  EXPECT_EQ(expiration, DateTimeUtils::parse_expiration("2y", now));
}

TEST(DateTimeUtilsTest, parse_exception_relative_nonpositive) {
  auto now = std::chrono::system_clock::now();
  try {
    DateTimeUtils::parse_expiration("0y", now);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Unit count should be a positive integer.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
}

TEST(DateTimeUtilsTest, parse_expiration_invalid) {
  auto now = std::chrono::system_clock::now();
  try {
    DateTimeUtils::parse_expiration("-30d", now);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Invalid expiration time argument.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
  try {
    DateTimeUtils::parse_expiration("70n", now);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Invalid expiration time argument.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
  try {
    DateTimeUtils::parse_expiration("2026 01 01 10:00:00", now);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Invalid expiration time argument.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
  try {
    DateTimeUtils::parse_expiration("2026_01_01T10:00:00", now);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Invalid expiration time argument.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
  try {
    DateTimeUtils::parse_expiration("2026-01-01 10:00:00", now);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Invalid expiration time argument.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
  try {
    DateTimeUtils::parse_expiration("2026-01-01", now);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Invalid expiration time argument.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
  try {
    DateTimeUtils::parse_expiration("2026-01-01T10:00:00", now);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Invalid expiration time argument.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
  try {
    DateTimeUtils::parse_expiration("", now);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Invalid expiration time argument.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
}
