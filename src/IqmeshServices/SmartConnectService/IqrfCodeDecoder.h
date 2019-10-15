#pragma once
#include <string>

namespace iqrf {
  
  // Decodes encoded IQRF Code 
  class IqrfCodeDecoder {
  private:
    static std::basic_string<uint8_t> m_mid;
    static std::basic_string<uint8_t> m_ibk;
    static uint16_t m_hwpId;

  public:

    // decodes specified IQRF Code
    // must be called BEFORE getters of decoded values
    // throws exception, if some error occurred during decoding
    static void decode(const std::string& iqrfCode);

    // returns decoded MID
    static std::basic_string<uint8_t> getMid() { return m_mid; };

    // returns decoded IBK
    static std::basic_string<uint8_t> getIbk() { return m_ibk; };

    // returns decoded HWP id
    static uint16_t getHwpId() { return m_hwpId; };
  };
}
