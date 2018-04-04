#pragma once

#include <cstdint>

// Configuration byte - according to DPA spec
struct HWP_ConfigByte {
  uint8_t address;
  uint8_t value;
  uint8_t mask;

  HWP_ConfigByte() {
    address = 0;
    value = 0;
    mask = 0;
  }

  HWP_ConfigByte(uint8_t addressVal, uint8_t valueVal, uint8_t maskVal) {
    address = addressVal;
    value = valueVal;
    mask = maskVal;
  }
};