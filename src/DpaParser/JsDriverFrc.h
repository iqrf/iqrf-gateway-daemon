#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "EmbedFRC.h"
#include "JsonUtils.h"

namespace iqrf
{
  namespace embed
  {
    namespace frc
    {
      ////////////////
      class JsDriverSend : public Send, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverSend(IJsRenderService* iJsRenderService, uint8_t frcCommand, const std::vector<uint8_t> & userData)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , Send(frcCommand, userData)
        {}

        virtual ~JsDriverSend() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.frc.Send";
        }

        void requestParameter(rapidjson::Document& par) const override
        {
          using namespace rapidjson;

          Pointer("/frcCommand").Set(par, (int)m_frcCommand);

          Value userDataArr;
          userDataArr.SetArray();
          for (auto n : m_userData) {
            Value nVal;
            nVal.SetInt(n);
            userDataArr.PushBack(nVal, par.GetAllocator());
          }
          Pointer("/userData").Set(par, userDataArr);
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;

          m_status = jutils::getMemberAs<int>("status", v);
          m_frcData = jutils::getMemberAsVector<int>("frcData", v);
        }

      };
      typedef std::unique_ptr<JsDriverSend> JsDriverSendPtr;

      ////////////////
      class JsDriverSendSelective : public SendSelective, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverSendSelective(IJsRenderService* iJsRenderService, uint8_t frcCommand, const std::set<int> & selectedNodes, const std::vector<uint8_t> & userData)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , SendSelective(frcCommand, selectedNodes, userData)
        {}

        virtual ~JsDriverSendSelective() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.frc.SendSelective";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;

          m_status = jutils::getMemberAs<int>("status", v);
          m_frcData = jutils::getMemberAsVector<int>("frcData", v);
        }
      };
      typedef std::unique_ptr<JsDriverSend> JsDriverSendPtr;

      ////////////////
      class JsDriverExtraResult : public ExtraResult, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverExtraResult(IJsRenderService* iJsRenderService)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , ExtraResult()
        {}

        virtual ~JsDriverExtraResult() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.frc.ExtraResult";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;

          m_frcData = jutils::getMemberAsVector<int>("frcData", v);
        }
      };
      typedef std::unique_ptr<JsDriverExtraResult> JsDriverExtraResultPtr;

    } //namespace frc
  } //namespace embed
} //namespace iqrf
