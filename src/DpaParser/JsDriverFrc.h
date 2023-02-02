/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
      class JsDriverSendSelective1 : public SendSelective, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverSendSelective1(IJsRenderService* iJsRenderService, uint8_t frcCommand, const std::set<int> & selectedNodes, const std::vector<uint8_t> & userData)
          :JsDriverDpaCommandSolver(iJsRenderService)
          , SendSelective(frcCommand, selectedNodes, userData)
        {}

        virtual ~JsDriverSendSelective1() {}

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
