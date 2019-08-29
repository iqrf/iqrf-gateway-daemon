#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "JsDriverStandardFrcSolver.h"
#include "Dali.h"
#include "JsonUtils.h"
#include <vector>

namespace iqrf
{
  namespace light
  {
    namespace jsdriver
    {
      namespace item {
        class Answer : public light::item::Answer
        {
        public:
          Answer(const rapidjson::Value& v)
            : light::item::Answer()
          {
            m_status = (uint8_t)jutils::getMemberAs<int>("status", v);
            m_value = (uint8_t)jutils::getMemberAs<int>("value", v);
          }
        };
      } //namespace item

      ////////////////
      class SendCommands : public light::SendCommands, public JsDriverDpaCommandSolver
      {
      public:
        SendCommands(IJsRenderService* iJsRenderService, const std::vector<uint16_t> & commands)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , light::SendCommands(commands)
        {}

        virtual ~SendCommands() {}

        static void parseAnswers(std::vector<light::item::AnswerPtr> & answers, const rapidjson::Value& v)
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
            answers.push_back(light::item::AnswerPtr(shape_new light::jsdriver::item::Answer(*itr)));
          }
        }

      protected:
        std::string functionName() const override
        {
          return "iqrf.light.SendCommandsAsync";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          jsdriver::SendCommands::parseAnswers(m_answers, v);
        }
      };
      typedef std::unique_ptr<SendCommands> SendCommandsPtr;

      ////////////////
      class SendCommandsAsync : public light::SendCommandsAsync, public JsDriverDpaCommandSolver
      {
      public:
        SendCommandsAsync(IJsRenderService* iJsRenderService, const std::vector<uint16_t> & commands)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , light::SendCommandsAsync(commands)
        {}

        virtual ~SendCommandsAsync() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.light.SendCommandsAsync";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          jsdriver::SendCommands::parseAnswers(m_answers, v);
        }

      };
      typedef std::unique_ptr<SendCommands> SendCommandsPtr;

      ////////////////
      class Frc : public light::Frc, public JsDriverStandardFrcSolver
      {
      public:

        Frc(IJsRenderService* iJsRenderService, int command)
          :JsDriverStandardFrcSolver(iJsRenderService)
          , light::Frc(command)
        {}

        Frc(IJsRenderService* iJsRenderService, int command, const std::vector<int> & selectedNodes)
          :JsDriverStandardFrcSolver(iJsRenderService)
          , light::Frc(command, selectedNodes)
        {}

        virtual ~Frc() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.light.Frc";
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
  } //namespace light
} //namespace iqrf
