/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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
#pragma once
#include <algorithm>
#include <cstdint>

#include "Base57.h"
#include "DPA.h"
#include "StreamUtils.h"

/// IQRF code namespace
namespace iqrf::code {

  /**
   * IQRF values tags
   */
  enum class Tags {
    /**
     * End of stream
     */
    END,
    /**
     * Module ID
     */
    MID,
    /**
     * Individual bonding key
     */
    IBK,
    /**
     * Hardware profile ID
     */
    HWPID,
    /**
     * Logical address in IQMESH network
     */
    LOGICAL_ADDRESS,
    /**
     * NO operation, used to align data in stream
     */
    NOP,
    /**
     * Used to store custom dat.
     */
    DATA_BLOCK,
    /**
     * Used to store custom text
     */
    TEXT,
    /**
     * Hardware profile ID version
     */
    HWPID_VER
  };

  class IqrfValues {
  private:
    /// Module ID
    uint32_t mid;
    /// Individual bonding key
    std::basic_string<uint8_t> ibk;
    /// Hardware profile ID
    uint16_t hwpid;
  public:
    /**
     * Constructor for smart connect code
     * @param mid Module ID
     * @param ibk Individual bonding key
     * @param hwpid Hardware profile ID
     */
    IqrfValues(uint32_t mid, std::basic_string<uint8_t> ibk, uint16_t hwpid): mid(mid), ibk(ibk), hwpid(hwpid) {}

    /**
     * Returns module ID as integer value
     * @return Module ID value
     */
    uint32_t getMid() const {
      return mid;
    }

    /**
     * Returns module ID as bytes
     * @return Module ID btytes
     */
    std::basic_string<uint8_t> getMidBytes() {
      return {
        static_cast<uint8_t>((mid >> 24) & 0xFF),
        static_cast<uint8_t>((mid >> 16) & 0xFF),
        static_cast<uint8_t>((mid >> 8) & 0xFF),
        static_cast<uint8_t>(mid & 0xFF)
      };
    }

    /**
     * Returns individual bonding key bytes
     * @return Individual bonding key
     */
    std::basic_string<uint8_t> getIbk() const {
      return ibk;
    }

    /**
     * Returns hardware profile ID value
     * @return Hardware profile ID value
     */
    uint16_t getHwpid() const {
      return hwpid;
    }

    /**
     * Returns hardware profile ID bytes
     * @return Hardware profile ID bytes
     */
    std::basic_string<uint8_t> getHwpidBytes() {
      return {
        static_cast<uint8_t>((hwpid >> 8) & 0xFF),
        static_cast<uint8_t>(hwpid & 0xFF)
      };
    }
  };

  /**
   * Encodes IQRF values to IQRF code
   * @param values IQRF values to encode
   * @return IQRF code with encoded values
   */
  static std::string encode(IqrfValues values) {
    utils::stream::StreamWriter writer;

    // Write MID
    writer.writeNibbleAndBytes(
      static_cast<uint8_t>(Tags::MID),
      values.getMidBytes()
    );
    // Write IBK
    writer.writeNibbleAndBytes(
      static_cast<uint8_t>(Tags::IBK),
      values.getIbk()
    );
    // Write HWPID
    writer.writeNibbleAndBytes(
      static_cast<uint8_t>(Tags::HWPID),
      values.getHwpidBytes()
    );
    // Write END tag
    writer.writeNibble(static_cast<uint8_t>(Tags::END));
    auto data = writer.finalize();

    return base57::encode(data);
  }

  /**
   * Decodes IQRF code into MID, IBK and HWPID
   * @param iqrfCode IQRF code
   */
  static IqrfValues decode(const std::string& iqrfCode) {
    if (iqrfCode.empty()) {
      throw std::logic_error("IQRF code is empty (length 0).");
    }

    if (!base57::validateCheckCharacter(iqrfCode)) {
      throw std::logic_error("IQRF code contains incorrect check character.");
    }

    std::string code = iqrfCode.substr(0, iqrfCode.length() - 1);
    auto data = base57::decode(code);

    utils::stream::StreamReader reader(data);
    uint32_t mid = 0;
    std::basic_string<uint8_t> ibk;
    uint16_t hwpid = 0;

    for (Tags tag; (tag = (Tags)reader.readNibble()) != Tags::END; ) {
      switch (tag) {
        case Tags::MID:
          mid = reader.readByte() << 24 | reader.readByte() << 16 | reader.readByte() << 8 | reader.readByte();
          break;
        case Tags::IBK:
          ibk = reader.readBytes(16);
          break;
        case Tags::HWPID:
          hwpid = reader.readByte() << 8 | reader.readByte();
          break;
        case Tags::LOGICAL_ADDRESS:
        case Tags::NOP:
        case Tags::DATA_BLOCK:
        case Tags::TEXT:
        case Tags::HWPID_VER:
          break;
        default:
          throw std::logic_error("Unknown tag value.");
      }
    }

    return IqrfValues(mid, ibk, hwpid);
  }
}
