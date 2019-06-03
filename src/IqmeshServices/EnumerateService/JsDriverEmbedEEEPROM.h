#pragma once

#include "JsDriverRequest.h"
#include "JsonUtils.h"
#include <vector>

namespace iqrf 
{
  namespace embed
  {
    namespace eeeprom
    {
      ////////////////
      class Read : public JsDriverRequest
      {
      private:
        //params
        uint16_t m_address;
        uint8_t m_len;

        //response
        std::vector<int> m_pdata;

      public:
        Read(uint16_t nadr, uint16_t address, uint8_t len)
          :JsDriverRequest(nadr)
          ,m_address(address)
          ,m_len(len)
        {
        }

        std::string functionName() const override
        {
          return "iqrf.embed.eeeprom.Read";
        }

        std::string requestParameter() const override
        {
          using namespace rapidjson;
          Document par;

          Pointer("/address").Set(par, (int)m_address);
          Pointer("/len").Set(par, (int)m_len);

          std::string parStr;
          StringBuffer buffer;
          Writer<rapidjson::StringBuffer> writer(buffer);
          par.Accept(writer);
          parStr = buffer.GetString();

          return parStr;
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          //TODO use rapidjson::pointers ?
          m_pdata = jutils::getMemberAsVector<int>("pData", v);
        }

        // get data as returned from driver
        const std::vector<int> & getPdata() const { return m_pdata; }

        // get more detailed data parsing

      };

    } //namespace eeeprom
  } //namespace embed
} //namespace iqrf
