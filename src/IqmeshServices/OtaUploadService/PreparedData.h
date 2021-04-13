#pragma once

#include <vector>

namespace iqrf
{
  /// \class PreparedData
  /// \brief Holds data prepared for writing into external EEPROM and loading into
  /// MCU Flash memory on nodes.
  class PreparedData
  {
  public:
    typedef std::vector<std::basic_string<uint8_t>> Data;

    virtual Data getData() = 0;
    virtual uint16_t getChecksum() = 0;
    virtual uint16_t getLength() = 0;
  };
}