#include <gtest/gtest.h>
#include "DateTimeUtils.h"

#include <thread>

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

TEST(DateTimeUtilsTest, get_current_timestamp_no_delay) {
  auto before = std::chrono::system_clock::now();
  auto ts = DateTimeUtils::get_current_timestamp();
  auto after = std::chrono::system_clock::now();

  auto before_epoch = std::chrono::duration_cast<std::chrono::seconds>(before.time_since_epoch()).count();
  auto after_epoch = std::chrono::duration_cast<std::chrono::seconds>(after.time_since_epoch()).count();

  EXPECT_LE(before_epoch, ts);
  EXPECT_GE(after_epoch, ts);
}

TEST(DateTimeUtilsTest, get_current_timestamp_delay) {
  auto before = std::chrono::system_clock::now();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  auto ts = DateTimeUtils::get_current_timestamp();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  auto after = std::chrono::system_clock::now();

  auto before_epoch = std::chrono::duration_cast<std::chrono::seconds>(before.time_since_epoch()).count();
  auto after_epoch = std::chrono::duration_cast<std::chrono::seconds>(after.time_since_epoch()).count();

  EXPECT_LE(before_epoch, ts);
  EXPECT_GE(after_epoch, ts);
}

TEST(DateTimeUtilsTest, parse_expiration_unix_timestamp_valid) {
  EXPECT_EQ(1762281501, DateTimeUtils::parse_expiration("1762281501", 1762281415));
}

TEST(DateTimeUtilsTest, parse_expiration_unix_timestamp_past) {
  try {
    DateTimeUtils::parse_expiration("1762281501", 1762282475);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Expiration unix timestamp is in the past.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
}

TEST(DateTimeUtilsTest, parse_expiration_relative_valid) {
  EXPECT_EQ(1731590400, DateTimeUtils::parse_expiration("10d", 1730726400));
  EXPECT_EQ(1731331200, DateTimeUtils::parse_expiration("1w", 1730726400));
  EXPECT_EQ(1738502400, DateTimeUtils::parse_expiration("3m", 1730726400));
  EXPECT_EQ(1793798400, DateTimeUtils::parse_expiration("2y", 1730726400));
}

TEST(DateTimeUtilsTest, parse_exception_relative_nonpositive) {
  try {
    DateTimeUtils::parse_expiration("0y", 1762282475);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Unit count should be a positive integer.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
}

TEST(DateTimeUtilsTest, parse_expiration_invalid) {
  try {
    DateTimeUtils::parse_expiration("-30d", 1762282475);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Invalid expiration time argument.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
  try {
    DateTimeUtils::parse_expiration("70n", 1762282475);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Invalid expiration time argument.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
  try {
    DateTimeUtils::parse_expiration("1762d282475m", 1762282475);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Invalid expiration time argument.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
  try {
    DateTimeUtils::parse_expiration("", 1762282475);
    FAIL() << "Expected std::invalid_argument to be thrown, but no exception was thrown.";
  } catch (const std::invalid_argument& e) {
    EXPECT_STREQ("Invalid expiration time argument.", e.what());
  } catch (...) {
    FAIL() << "Expected std::invalid_argument to be thrown, but a different exception type was thrown instead.";
  }
}
