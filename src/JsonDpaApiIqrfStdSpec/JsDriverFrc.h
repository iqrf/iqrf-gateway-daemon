#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "Frc.h"
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
        JsDriverSendSelective(IJsRenderService* iJsRenderService, uint8_t frcCommand, const std::vector<int> & selectedNodes, const std::vector<uint8_t> & userData)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , SendSelective(frcCommand, selectedNodes, userData)
        {}

        virtual ~JsDriverSendSelective() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.frc.SendSelective";
        }

        void requestParameter(rapidjson::Document& par) const override
        {
          using namespace rapidjson;

          Value selectedNodesArr;
          selectedNodesArr.SetArray();
          for (auto n : m_selectedNodes) {
            Value nVal;
            nVal.SetInt(n);
            selectedNodesArr.PushBack(nVal, par.GetAllocator());
          }
          Pointer("/selectedNodes").Set(par, selectedNodesArr);
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;

          m_status = jutils::getMemberAs<int>("status", v);
          m_frcData = jutils::getMemberAsVector<int>("frcData", v);
        }
      };
      typedef std::unique_ptr<JsDriverSend> JsDriverSendPtr;

    } //namespace frc
  } //namespace embed
} //namespace iqrf
