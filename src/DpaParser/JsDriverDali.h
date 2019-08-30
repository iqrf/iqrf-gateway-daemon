#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "JsDriverStandardFrcSolver.h"
#include "Dali.h"
#include "JsonUtils.h"
#include <vector>

namespace iqrf
{
  namespace dali
  {
    namespace jsdriver
    {
      namespace item {
        class Answer : public dali::item::Answer
        {
        public:
          Answer(const rapidjson::Value& v)
            : dali::item::Answer()
          {
            m_status = (uint8_t)jutils::getMemberAs<int>("status", v);
            m_value = (uint8_t)jutils::getMemberAs<int>("value", v);
          }
        };
      } //namespace item

      ////////////////
      class SendCommands : public dali::SendCommands, public JsDriverDpaCommandSolver
      {
      public:
        SendCommands(IJsRenderService* iJsRenderService, const std::vector<uint16_t> & commands)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , dali::SendCommands(commands)
        {}

        virtual ~SendCommands() {}

        static void parseAnswers(std::vector<dali::item::AnswerPtr> & answers, const rapidjson::Value& v)
        {
          using namespace rapidjson;

          const Value *arrayVal = Pointer("/answers").Get(v);
          if (!(arrayVal && arrayVal->IsArray())) {
            THROW_EXC_TRC_WAR(std::logic_error, "Expected: Json Array .../answers[]");
          }

          for (const Value *itr = arrayVal->Begin(); itr != arrayVal->End(); ++itr) {
            if (!(itr && itr->IsObject())) {
              THROW_EXC_TRC_WAR(std::logic_error, "Expected: Object item in .../answers[{}]");
            }
            answers.push_back(dali::item::AnswerPtr(shape_new dali::jsdriver::item::Answer(*itr)));
          }
        }

      protected:
        std::string functionName() const override
        {
          return "iqrf.dali.SendCommandsAsync";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          jsdriver::SendCommands::parseAnswers(m_answers, v);
        }
      };
      typedef std::unique_ptr<SendCommands> SendCommandsPtr;

      ////////////////
      class SendCommandsAsync : public dali::SendCommandsAsync, public JsDriverDpaCommandSolver
      {
      public:
        SendCommandsAsync(IJsRenderService* iJsRenderService, const std::vector<uint16_t> & commands)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , dali::SendCommandsAsync(commands)
        {}

        virtual ~SendCommandsAsync() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.dali.SendCommandsAsync";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          jsdriver::SendCommands::parseAnswers(m_answers, v);
        }

      };
      typedef std::unique_ptr<SendCommands> SendCommandsPtr;

      ////////////////
      class Frc : public dali::Frc, public JsDriverStandardFrcSolver
      {
      public:

        Frc(IJsRenderService* iJsRenderService, int command)
          :JsDriverStandardFrcSolver(iJsRenderService)
          , dali::Frc(command)
        {}

        Frc(IJsRenderService* iJsRenderService, int command, const std::vector<int> & selectedNodes)
          :JsDriverStandardFrcSolver(iJsRenderService)
          , dali::Frc(command, selectedNodes)
        {}

        virtual ~Frc() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.dali.Frc";
        }

        void preRequest(rapidjson::Document& requestParamDoc) override
        {
          using namespace rapidjson;

          Pointer("/command").Set(requestParamDoc, (int)m_daliCommand);

          Value selectedNodesArr;
          selectedNodesArr.SetArray();
          for (auto n : m_selectedNodes) {
            Value nVal;
            nVal.SetInt(n);
            selectedNodesArr.PushBack(nVal, requestParamDoc.GetAllocator());
          }
          Pointer("/selectedNodes").Set(requestParamDoc, selectedNodesArr);
        }

        void postResponse(const rapidjson::Document& responseResultDoc) override
        {
          JsDriverStandardFrcSolver::postResponse(responseResultDoc);
          jsdriver::SendCommands::parseAnswers(m_answers, responseResultDoc);
        }

      };
      typedef std::unique_ptr<Frc> FrcPtr;
    } //namespace jsdriver
  } //namespace dali
} //namespace iqrf
