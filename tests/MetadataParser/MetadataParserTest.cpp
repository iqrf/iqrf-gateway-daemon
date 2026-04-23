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

#include "MetadataParser.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

using json = nlohmann::json;

namespace iqrf::metadata {

  TEST(MetadataParserTest, parseV0_specified) {
    json doc({
      {"hwpidVersions", {
        {"min", 1},
        {"max", -1},
      }},
      {"routing", true},
      {"beaming", false},
      {"repeater", true},
      {"frcAggregation", true},
      {"iqarosCompatible", true},
      {"iqrfSensor", {
        1, 128, 128
      }},
      {"iqrfBinaryOutput", 4},
      {"powerSupply", {
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
      }}
    });

    auto parsed = MetadataParser::parseV0(doc);

    // hwpid versions
    EXPECT_EQ(1, parsed.versions().min());
    EXPECT_FALSE(parsed.versions().max().has_value());
    // generic properties
    EXPECT_TRUE(parsed.routing());
    EXPECT_FALSE(parsed.beaming());
    EXPECT_TRUE(parsed.repeater());
    EXPECT_TRUE(parsed.frcAggregation());
    EXPECT_TRUE(parsed.iqarosCompatible());
    EXPECT_EQ(3, parsed.sensors().size());
    EXPECT_EQ(1, parsed.sensors()[0]);
    EXPECT_EQ(128, parsed.sensors()[1]);
    EXPECT_EQ(128, parsed.sensors()[2]);
    EXPECT_EQ(4, parsed.binaryOutputs());
    // power supply
    EXPECT_TRUE(parsed.powerSupply().mains());
    EXPECT_TRUE(parsed.powerSupply().accumulator().present());
    ASSERT_TRUE(parsed.powerSupply().accumulator().type().has_value());
    EXPECT_EQ("LIP552240", parsed.powerSupply().accumulator().type().value());
    ASSERT_TRUE(parsed.powerSupply().accumulator().lowLevel().has_value());
    EXPECT_EQ(3.4, parsed.powerSupply().accumulator().lowLevel().value());
    EXPECT_TRUE(parsed.powerSupply().battery().present());
    ASSERT_TRUE(parsed.powerSupply().battery().type().has_value());
    EXPECT_EQ("CR2450", parsed.powerSupply().battery().type().value());
    ASSERT_TRUE(parsed.powerSupply().battery().changeThreshold().has_value());
    EXPECT_EQ(2.4, parsed.powerSupply().battery().changeThreshold().value());
    EXPECT_FALSE(parsed.powerSupply().battery().conditioning().has_value());
    EXPECT_EQ(2.2, parsed.powerSupply().minVoltage());
  }

  TEST(MetadataParserTest, parseV1_specified) {
    json doc({
      {"hwpidVersions", {
        {"min", 1},
        {"max", -1},
      }},
      {"routing", true},
      {"beaming", false},
      {"repeater", true},
      {"frcAggregation", true},
      {"iqarosCompatible", true},
      {"iqrfSensor", {
        1, 128, 128
      }},
      {"iqrfBinaryOutput", 4},
      {"iqrfLight", {
        {"type", "LDI"},
        {"socketType", "Terminals"}
      }},
      {"minMaxDiag", {
        {"version", 1}
      }},
      {"powerSupply", {
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
      }},
      {"actionRecord", {
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
      }},
      {"persistentQuantities", {
        {
          {"quantity", 1},
          {"value", 75},
          {"description", "Temperature test"},
          {"memory", {
            {"type", 5},
            {"addr", 10}
          }}
        }
      }},
      {"nfcc", {
        {"version", 1}
      }},
      {"mfgc", {
        {"version", 1}
      }},
      {"calibration", {
        {"version", 1}
      }}
    });

    auto parsed = MetadataParser::parseV1(doc);

    // hwpid versions
    EXPECT_EQ(1, parsed.versions().min());
    EXPECT_FALSE(parsed.versions().max().has_value());
    // generic properties
    EXPECT_TRUE(parsed.routing());
    EXPECT_FALSE(parsed.beaming());
    EXPECT_TRUE(parsed.repeater());
    EXPECT_TRUE(parsed.frcAggregation());
    EXPECT_TRUE(parsed.iqarosCompatible());
    EXPECT_EQ(3, parsed.sensors().size());
    EXPECT_EQ(1, parsed.sensors()[0]);
    EXPECT_EQ(128, parsed.sensors()[1]);
    EXPECT_EQ(128, parsed.sensors()[2]);
    EXPECT_EQ(4, parsed.binaryOutputs());
    // light
    ASSERT_TRUE(parsed.light().has_value());
    ASSERT_TRUE(parsed.light().value().type().has_value());
    EXPECT_EQ("LDI", parsed.light().value().type().value());
    ASSERT_TRUE(parsed.light().value().socketType().has_value());
    EXPECT_EQ("Terminals", parsed.light().value().socketType().value());
    // min max diag
    ASSERT_TRUE(parsed.minMaxDiag().has_value());
    ASSERT_TRUE(parsed.minMaxDiag().value().version().has_value());
    EXPECT_EQ(1, parsed.minMaxDiag().value().version().value());
    // power supply
    EXPECT_TRUE(parsed.powerSupply().mains());
    EXPECT_TRUE(parsed.powerSupply().accumulator().present());
    ASSERT_TRUE(parsed.powerSupply().accumulator().type().has_value());
    EXPECT_EQ("LIP552240", parsed.powerSupply().accumulator().type().value());
    ASSERT_TRUE(parsed.powerSupply().accumulator().lowLevel().has_value());
    EXPECT_EQ(3.4, parsed.powerSupply().accumulator().lowLevel().value());
    EXPECT_TRUE(parsed.powerSupply().battery().present());
    ASSERT_TRUE(parsed.powerSupply().battery().type().has_value());
    EXPECT_EQ("CR2450", parsed.powerSupply().battery().type().value());
    ASSERT_TRUE(parsed.powerSupply().battery().changeThreshold().has_value());
    EXPECT_EQ(2.4, parsed.powerSupply().battery().changeThreshold().value());
    EXPECT_FALSE(parsed.powerSupply().battery().conditioning().has_value());
    EXPECT_EQ(2.2, parsed.powerSupply().minVoltage());
    // action record
    ASSERT_TRUE(parsed.actionRecord().has_value());
    ASSERT_TRUE(parsed.actionRecord().value().memory().has_value());
    ASSERT_TRUE(parsed.actionRecord().value().memory().value().type().has_value());
    EXPECT_EQ(5, parsed.actionRecord().value().memory().value().type().value());
    ASSERT_TRUE(parsed.actionRecord().value().memory().value().address().has_value());
    EXPECT_EQ(10, parsed.actionRecord().value().memory().value().address().value());
    ASSERT_TRUE(parsed.actionRecord().value().memory().value().size().has_value());
    EXPECT_EQ(30, parsed.actionRecord().value().memory().value().size().value());
    ASSERT_EQ(1, parsed.actionRecord().value().commands().size());
    ASSERT_TRUE(parsed.actionRecord().value().commands()[0].value().has_value());
    EXPECT_EQ(0, parsed.actionRecord().value().commands()[0].value());
    ASSERT_TRUE(parsed.actionRecord().value().commands()[0].text().has_value());
    EXPECT_EQ("Do nothing", parsed.actionRecord().value().commands()[0].text());
    // persistent quantities
    ASSERT_EQ(1, parsed.peristentQuantities().size());
    ASSERT_TRUE(parsed.peristentQuantities()[0].quantity().has_value());
    EXPECT_EQ(1, parsed.peristentQuantities()[0].quantity().value());
    ASSERT_TRUE(parsed.peristentQuantities()[0].value().has_value());
    EXPECT_EQ(75, parsed.peristentQuantities()[0].value().value());
    ASSERT_TRUE(parsed.peristentQuantities()[0].description().has_value());
    EXPECT_EQ("Temperature test", parsed.peristentQuantities()[0].description().value());
    ASSERT_TRUE(parsed.peristentQuantities()[0].quantityMemory().has_value());
    ASSERT_TRUE(parsed.peristentQuantities()[0].quantityMemory().value().type().has_value());
    EXPECT_EQ(5, parsed.peristentQuantities()[0].quantityMemory().value().type().value());
    ASSERT_TRUE(parsed.peristentQuantities()[0].quantityMemory().value().address().has_value());
    EXPECT_EQ(10, parsed.peristentQuantities()[0].quantityMemory().value().address().value());
    // nfcc
    ASSERT_TRUE(parsed.nfcc().has_value());
    ASSERT_TRUE(parsed.nfcc().value().version().has_value());
    EXPECT_EQ(1, parsed.nfcc().value().version().value());
    // mfgc
    ASSERT_TRUE(parsed.mfgc().has_value());
    ASSERT_TRUE(parsed.mfgc().value().version().has_value());
    EXPECT_EQ(1, parsed.mfgc().value().version().value());
    // calibration
    ASSERT_TRUE(parsed.calibration().has_value());
    ASSERT_TRUE(parsed.calibration().value().version().has_value());
    EXPECT_EQ(1, parsed.calibration().value().version().value());
  }

  class MetadataParserParameterizedErrorTest : public ::testing::TestWithParam<json> {};

  TEST_P(MetadataParserParameterizedErrorTest, parse_missing_fields) {
    auto doc = GetParam();
    EXPECT_THROW(MetadataParser::parseV0(doc), std::invalid_argument);
    EXPECT_THROW(MetadataParser::parseV1(doc), std::invalid_argument);
  }

  INSTANTIATE_TEST_CASE_P(
    MetadataParserTest,
    MetadataParserParameterizedErrorTest,
    ::testing::Values(
      json({
        {"routing", true},
        {"beaming", false},
        {"repeater", true},
        {"frcAggregation", true},
        {"iqarosCompatible", true},
        {"iqrfSensor", {
          1, 128, 128
        }},
        {"iqrfBinaryOutput", 4},
        {"powerSupply", {
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
        }}
      }),
      json({
        {"hwpidVersions", {
          {"min", 1},
          {"max", -1},
        }},
        {"beaming", false},
        {"repeater", true},
        {"frcAggregation", true},
        {"iqarosCompatible", true},
        {"iqrfSensor", {
          1, 128, 128
        }},
        {"iqrfBinaryOutput", 4},
        {"powerSupply", {
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
        }}
      }),
      json({
        {"hwpidVersions", {
          {"min", 1},
          {"max", -1},
        }},
        {"routing", true},
        {"repeater", true},
        {"frcAggregation", true},
        {"iqarosCompatible", true},
        {"iqrfSensor", {
          1, 128, 128
        }},
        {"iqrfBinaryOutput", 4},
        {"powerSupply", {
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
        }}
      }),
      json({
        {"hwpidVersions", {
          {"min", 1},
          {"max", -1},
        }},
        {"routing", true},
        {"beaming", true},
        {"frcAggregation", true},
        {"iqarosCompatible", true},
        {"iqrfSensor", {
          1, 128, 128
        }},
        {"iqrfBinaryOutput", 4},
        {"powerSupply", {
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
        }}
      }),
      json({
        {"hwpidVersions", {
          {"min", 1},
          {"max", -1},
        }},
        {"routing", true},
        {"beaming", false},
        {"repeater", true},
        {"iqarosCompatible", true},
        {"iqrfSensor", {
          1, 128, 128
        }},
        {"iqrfBinaryOutput", 4},
        {"powerSupply", {
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
        }}
      }),
      json({
        {"hwpidVersions", {
          {"min", 1},
          {"max", -1},
        }},
        {"routing", true},
        {"beaming", false},
        {"repeater", true},
        {"frcAggregation", true},
        {"iqrfSensor", {
          1, 128, 128
        }},
        {"iqrfBinaryOutput", 4},
        {"powerSupply", {
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
        }}
      }),
      json({
        {"hwpidVersions", {
          {"min", 1},
          {"max", -1},
        }},
        {"routing", true},
        {"beaming", false},
        {"repeater", true},
        {"frcAggregation", true},
        {"iqarosCompatible", true},
        {"iqrfBinaryOutput", 4},
        {"powerSupply", {
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
        }}
      }),
      json({
        {"hwpidVersions", {
          {"min", 1},
          {"max", -1},
        }},
        {"routing", true},
        {"beaming", false},
        {"repeater", true},
        {"frcAggregation", true},
        {"iqarosCompatible", true},
        {"iqrfSensor", {
          1, 128, 128
        }},
        {"powerSupply", {
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
        }}
      }),
      json({
        {"hwpidVersions", {
          {"min", 1},
          {"max", -1},
        }},
        {"routing", true},
        {"beaming", false},
        {"repeater", true},
        {"frcAggregation", true},
        {"iqarosCompatible", true},
        {"iqrfSensor", {
          1, 128, 128
        }},
        {"iqrfBinaryOutput", 4}
      })
    )
  );
}

