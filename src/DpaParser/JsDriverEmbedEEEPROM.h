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

      };

    } //namespace eeeprom
  } //namespace embed
} //namespace iqrf
