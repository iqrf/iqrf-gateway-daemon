#pragma once

#include <vector>

namespace iqrf
{
  namespace embed
  {
    namespace eeeprom
    {
      ////////////////
      class Read
      {
      protected:
        //params
        uint16_t m_address;
        uint8_t m_len;

        //response
        std::vector<int> m_pdata;

        Read()
        {}

        Read(uint16_t address, uint8_t len)
          : m_address(address)
          , m_len(len)
        {}

      public:
        virtual ~Read()
        {}

        // get read data
        const std::vector<int> & getPdata() const { return m_pdata; }
      };

    } //namespace eeeprom
  } //namespace embed
} //namespace iqrf
