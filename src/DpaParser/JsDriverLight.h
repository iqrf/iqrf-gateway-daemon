/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "JsDriverDpaCommandSolver.h"
#include "JsDriverStandardFrcSolver.h"
#include "Light.h"
#include "JsonUtils.h"
#include <vector>

namespace iqrf {
  namespace light {
    namespace jsdriver {
      namespace item {
        class Answer : public light::item::Answer {
        public:
          Answer(const rapidjson::Value& v) : light::item::Answer() {
            m_status = (uint8_t)jutils::getMemberAs<int>("status", v);
            m_value = (uint8_t)jutils::getMemberAs<int>("value", v);
          }
        };
      } //namespace item

      ////////////////
      class SendLdiCommands : public light::SendLdiCommands, public JsDriverDpaCommandSolver
      {
      public:
        SendLdiCommands(IJsRenderService* iJsRenderService, const std::vector<uint16_t> & ldiCommands)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , light::SendLdiCommands(ldiCommands)
        {}

        virtual ~SendLdiCommands() {}

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
        std::string functionName() const override {
          return "iqrf.light.SendLdiCommands";
        }

        void parseResponse(const rapidjson::Value& v) override {
          jsdriver::SendLdiCommands::parseAnswers(m_answers, v);
        }
      };
      typedef std::unique_ptr<SendLdiCommands> SendLdiCommandsPtr;

      ////////////////
      class SendLdiCommandsAsync : public light::SendLdiCommandsAsync, public JsDriverDpaCommandSolver {
      public:
        SendLdiCommandsAsync(IJsRenderService* iJsRenderService, const std::vector<uint16_t>& ldiCommands)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , light::SendLdiCommandsAsync(ldiCommands)
        {}

        virtual ~SendLdiCommandsAsync() {}

      protected:
        std::string functionName() const override {
          return "iqrf.light.SendLdiCommandsAsync";
        }

        void parseResponse(const rapidjson::Value& v) override {
          jsdriver::SendLdiCommands::parseAnswers(m_answers, v);
        }

      };
      typedef std::unique_ptr<SendLdiCommands> SendCommandsPtr;

      class SetLai : public light::SetLai, public JsDriverDpaCommandSolver {
      public:
        SetLai(IJsRenderService* jsRenderService, const uint16_t voltage)
        : JsDriverDpaCommandSolver(jsRenderService), light::SetLai(voltage) {}

        virtual ~SetLai() {}
      protected:
        std::string functionName() const override {
          return "iqrf.light.SetLai";
        }

        void parseResponse(const rapidjson::Value &v) override {
          m_prevVoltage = std::round(jutils::getMemberAs<double>("prevVoltage", v) * 1000);
        }
      };

      /*
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
      */

    } //namespace jsdriver
  } //namespace dali
} //namespace iqrf
