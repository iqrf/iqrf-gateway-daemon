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

#include "PowerSupplyParser.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

using json = nlohmann::json;

namespace iqrf::metadata {

  TEST(PowerSupplyParser, parseV0_specified) {
    json doc({
      {"mains", true},
      {"accumulator", {
        {"present", true},
        {"type", "LIP552240"},
        {"lowLevel", 3.4}
      }},
      {"battery", {
        {"present", true},
        {"type", "CR2450"},
        {"changeThreshold", 2.4}
      }},
      {"minVoltage", 2.2}
    });

    auto parsed = PowerSupplyParser::parseV0(doc);

    EXPECT_TRUE(parsed.mains());
    EXPECT_TRUE(parsed.accumulator().present());
    ASSERT_TRUE(parsed.accumulator().type().has_value());
    EXPECT_EQ("LIP552240", parsed.accumulator().type().value());
    ASSERT_TRUE(parsed.accumulator().lowLevel().has_value());
    EXPECT_EQ(3.4, parsed.accumulator().lowLevel().value());
    EXPECT_TRUE(parsed.battery().present());
    ASSERT_TRUE(parsed.battery().type().has_value());
    EXPECT_EQ("CR2450", parsed.battery().type().value());
    ASSERT_TRUE(parsed.battery().changeThreshold().has_value());
    EXPECT_EQ(2.4, parsed.battery().changeThreshold().value());
    EXPECT_FALSE(parsed.battery().conditioning().has_value());
    EXPECT_EQ(2.2, parsed.minVoltage());
  }

  TEST(PowerSupplyParser, parseV1_specified) {
    json doc({
      {"mains", true},
      {"accumulator", {
        {"present", true},
        {"type", "LIP552240"},
        {"lowLevel", 3.4}
      }},
      {"battery", {
        {"present", true},
        {"type", "CR2450"},
        {"changeThreshold", 2.4}
      }},
      {"external", {
        {"type", "DC"},
        {"nominalVoltage", 5},
        {"minVoltage", 3.75},
        {"maxVoltage", 6}
      }},
      {"minVoltage", 2.2}
    });

    auto parsed = PowerSupplyParser::parseV1(doc);

    EXPECT_TRUE(parsed.mains());
    EXPECT_TRUE(parsed.accumulator().present());
    ASSERT_TRUE(parsed.accumulator().type().has_value());
    EXPECT_EQ("LIP552240", parsed.accumulator().type().value());
    ASSERT_TRUE(parsed.accumulator().lowLevel().has_value());
    EXPECT_EQ(3.4, parsed.accumulator().lowLevel().value());
    EXPECT_TRUE(parsed.battery().present());
    ASSERT_TRUE(parsed.battery().type().has_value());
    EXPECT_EQ("CR2450", parsed.battery().type().value());
    ASSERT_TRUE(parsed.battery().changeThreshold().has_value());
    EXPECT_EQ(2.4, parsed.battery().changeThreshold().value());
    EXPECT_FALSE(parsed.battery().conditioning().has_value());
    ASSERT_TRUE(parsed.external().has_value());
    ASSERT_TRUE(parsed.external().value().type().has_value());
    EXPECT_EQ("DC", parsed.external().value().type().value());
    ASSERT_TRUE(parsed.external().value().nominalVoltage().has_value());
    EXPECT_EQ(5, parsed.external().value().nominalVoltage().value());
    ASSERT_TRUE(parsed.external().value().minVoltage().has_value());
    EXPECT_EQ(3.75, parsed.external().value().minVoltage().value());
    ASSERT_TRUE(parsed.external().value().maxVoltage().has_value());
    EXPECT_EQ(6, parsed.external().value().maxVoltage().value());
    EXPECT_EQ(2.2, parsed.minVoltage());
  }

  class PowerSupplyParserParameterizedErrorTest : public ::testing::TestWithParam<json> {};

  TEST_P(PowerSupplyParserParameterizedErrorTest, parse_missing_fields) {
    auto doc = GetParam();
    EXPECT_THROW(PowerSupplyParser::parseV0(doc), std::invalid_argument);
    EXPECT_THROW(PowerSupplyParser::parseV1(doc), std::invalid_argument);
  }

  INSTANTIATE_TEST_CASE_P(
    PowerSupplyParserTest,
    PowerSupplyParserParameterizedErrorTest,
    ::testing::Values(
    json({
        {"accumulator", {
          {"present", true},
          {"type", "LIP552240"},
          {"lowLevel", 3.4}
        }},
        {"battery", {
          {"present", true},
          {"type", "CR2450"},
          {"changeThreshold", 2.4}
        }},
        {"external", {
          {"type", "DC"},
          {"nominalVoltage", 5},
          {"minVoltage", 3.75},
          {"maxVoltage", 6}
        }},
        {"minVoltage", 2.2}
      }),
      json({
        {"mains", true},
        {"battery", {
          {"present", true},
          {"type", "CR2450"},
          {"changeThreshold", 2.4}
        }},
        {"external", {
          {"type", "DC"},
          {"nominalVoltage", 5},
          {"minVoltage", 3.75},
          {"maxVoltage", 6}
        }},
        {"minVoltage", 2.2}
      }),
      json({
        {"mains", true},
        {"accumulator", {
          {"present", true},
          {"type", "LIP552240"},
          {"lowLevel", 3.4}
        }},
        {"external", {
          {"type", "DC"},
          {"nominalVoltage", 5},
          {"minVoltage", 3.75},
          {"maxVoltage", 6}
        }},
        {"minVoltage", 2.2}
      }),
      json({
        {"mains", true},
        {"accumulator", {
          {"present", true},
          {"type", "LIP552240"},
          {"lowLevel", 3.4}
        }},
        {"battery", {
          {"present", true},
          {"type", "CR2450"},
          {"changeThreshold", 2.4}
        }},
        {"external", {
          {"type", "DC"},
          {"nominalVoltage", 5},
          {"minVoltage", 3.75},
          {"maxVoltage", 6}
        }}
      })
    )
  );

}
