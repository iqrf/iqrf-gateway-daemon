#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "EmbedEEEPROM.h"
#include "JsonUtils.h"

namespace iqrf 
{
  namespace embed
  {
    namespace eeeprom
    {
      ////////////////
      class JsDriverRead : public Read, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverRead(IJsRenderService* iJsRenderService, uint16_t nadr, uint16_t address, uint8_t len)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
          ,Read(address, len)
        {}

        virtual ~JsDriverRead()
        {}

        std::string functionName() const override
        {
          return "iqrf.embed.eeeprom.Read";
        }

        void requestParameter(rapidjson::Document& par) const override
        {
          using namespace rapidjson;
          Pointer("/address").Set(par, (int)m_address);
          Pointer("/len").Set(par, (int)m_len);
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          m_pdata = jutils::getMemberAsVector<int>("pData", v);
        }

      };

    } //namespace eeeprom
  } //namespace embed
} //namespace iqrf
