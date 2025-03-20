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
