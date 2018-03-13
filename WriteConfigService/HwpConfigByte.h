#pragma once
#include <cstdint>

/// Configuration byte - according to DPA spec
struct HWP_ConfigByte {
  uint8_t address;
  uint8_t value;
  uint8_t mask;
};