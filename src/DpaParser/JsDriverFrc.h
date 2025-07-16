/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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
          const Value *val = Pointer("/frcData").Get(v);
          if (!val || !val->IsArray()) {
            return;
          }
          auto arr = val->GetArray();
          std::vector<uint8_t> data;
          for (auto itr = arr.Begin(); itr != arr.End(); ++itr) {
            if (itr->IsUint()) {
              data.emplace_back(static_cast<uint8_t>(itr->GetUint()));
            }
          }
          m_frcData = data;
        }

      };
      typedef std::unique_ptr<JsDriverSend> JsDriverSendPtr;

      ////////////////
      class JsDriverSendSelective : public Send, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverSendSelective(IJsRenderService* iJsRenderService, uint8_t frcCommand, const std::set<uint8_t> &selectedNodes, const std::vector<uint8_t> &userData)
          :JsDriverDpaCommandSolver(iJsRenderService, 0)
          , Send(frcCommand, selectedNodes, userData)
        {}

        virtual ~JsDriverSendSelective() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.frc.SendSelective";
        }

        void requestParameter(rapidjson::Document& doc) const override
        {
          using namespace rapidjson;

          Document::AllocatorType &alloc = doc.GetAllocator();
          Pointer("/frcCommand").Set(doc, m_frcCommand);
          Value nodes(kArrayType);
          for (auto addr : m_selectedNodes) {
            Value val;
            val.SetUint(addr);
            nodes.PushBack(val, alloc);
          }
          Pointer("/selectedNodes").Set(doc, nodes);
          Value userData(kArrayType);
          for (auto data : m_userData) {
            Value val;
            val.SetUint(data);
            userData.PushBack(val, alloc);
          }
          Pointer("/userData").Set(doc, userData);
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;

          m_status = jutils::getMemberAs<int>("status", v);
          const Value *val = Pointer("/frcData").Get(v);
          if (!val || !val->IsArray()) {
            return;
          }
          auto arr = val->GetArray();
          std::vector<uint8_t> data;
          for (auto itr = arr.Begin(); itr != arr.End(); ++itr) {
            if (itr->IsUint()) {
              data.emplace_back(static_cast<uint8_t>(itr->GetUint()));
            }
          }
          m_frcData = data;
        }
      };
      typedef std::unique_ptr<JsDriverSendSelective> JsDriverSendSelectivePtr;

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

          const Value *val = Pointer("/frcData").Get(v);
          if (!val || !val->IsArray()) {
            return;
          }
          auto arr = val->GetArray();
          std::vector<uint8_t> data;
          for (auto itr = arr.Begin(); itr != arr.End(); ++itr) {
            if (itr->IsUint()) {
              data.emplace_back(static_cast<uint8_t>(itr->GetUint()));
            }
          }
          m_frcData = data;
        }
      };
      typedef std::unique_ptr<JsDriverExtraResult> JsDriverExtraResultPtr;

    } //namespace frc
  } //namespace embed
} //namespace iqrf
