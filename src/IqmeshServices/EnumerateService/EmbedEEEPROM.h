#pragma once

#include "JsDriver.h"

namespace iqrf 
{
  namespace embed
  {
    namespace eeeprom
    {
      ////////////////
      class Read : public JsDpaRequest
      {
      private:
        std::vector<int> m_pdata;

      public:
        Read(uint16_t nadr, uint16_t hwpid, uint16_t address, uint8_t len, IJsRenderService* iJsRenderService)
          :JsDpaRequest(nadr, hwpid, iJsRenderService)
        {
          rapidjson::Pointer("/address").Set(m_requestParameter, (int)address);
          rapidjson::Pointer("/len").Set(m_requestParameter, (int)len);
        }

        std::string getFunctionName() const override
        {
          return "iqrf.embed.eeeprom.Read";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;
          try {
            //TODO use rapidjson::pointers ?
            m_pdata = jutils::getMemberAsVector<int>("pData", v);
            m_valid = true;
          }
          catch (std::exception & e) {
            TRC_WARNING("invalid data: " << e.what());
            m_valid = false;
          }
        }

        // get data as returned from driver
        const std::vector<int> & getPdata() const { return m_pdata; }

        // get more detailed data parsing

      };

    } //namespace eeeprom
  } //namespace embed
} //namespace iqrf
