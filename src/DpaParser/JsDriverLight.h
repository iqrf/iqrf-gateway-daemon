/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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
#include "JsDriverStandardFrcSolver.h"
#include "Light.h"
#include "JsonUtils.h"
#include <vector>

namespace iqrf
{
  namespace light
  {
    namespace jsdriver
    {

      ////////////////
      class Enumerate : public light::Enumerate, public JsDriverDpaCommandSolver
      {
      public:
        Enumerate(IJsRenderService* iJsRenderService, uint16_t nadr)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
        {}

        virtual ~Enumerate() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.light.Enumerate";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          m_lightsNum = jutils::getMemberAs<int>("lights", v);
        }
      };
      typedef std::unique_ptr<Enumerate> JsDriverEnumeratePtr;

      ////////////////
      class SetPower : public light::SetPower, public JsDriverDpaCommandSolver
      {
      public:
        SetPower(IJsRenderService* iJsRenderService, const std::vector<light::item::Light> & lights)
          :light::SetPower(lights)
          , JsDriverDpaCommandSolver(iJsRenderService)
        {}

        virtual ~SetPower() {}

        static void parsePrevVals(std::vector<int> & prevVals, const rapidjson::Value& v)
        {
          using namespace rapidjson;

          const Value *arrayVal = Pointer("/prevVals").Get(v);
          if (!(arrayVal && arrayVal->IsArray())) {
            THROW_EXC_TRC_WAR(std::logic_error, "Expected: Json Array .../prevVals[]");
          }

          for (const Value *itr = arrayVal->Begin(); itr != arrayVal->End(); ++itr) {
            if (!(itr && itr->IsInt())) {
              THROW_EXC_TRC_WAR(std::logic_error, "Expected: integer item in .../prevVals[int]");
            }
            prevVals.push_back(itr->GetInt());
          }
        }

      protected:
        std::string functionName() const override
        {
          return "iqrf.light.SetPower";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          jsdriver::SetPower::parsePrevVals(m_prevVals, v);
        }
      };
      typedef std::unique_ptr<SetPower> SetPowerPtr;

      ////////////////
      class IncrementPower : public light::IncrementPower, public JsDriverDpaCommandSolver
      {
      public:
        IncrementPower(IJsRenderService* iJsRenderService, const std::vector<light::item::Light> & lights)
          :light::IncrementPower(lights)
          , JsDriverDpaCommandSolver(iJsRenderService)
        {}

        virtual ~IncrementPower() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.light.IncrementPower";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          jsdriver::SetPower::parsePrevVals(m_prevVals, v);
        }
      };
      typedef std::unique_ptr<IncrementPower> IncrementPowerPtr;

      ////////////////
      class DecrementPower : public light::DecrementPower, public JsDriverDpaCommandSolver
      {
      public:
        DecrementPower(IJsRenderService* iJsRenderService, const std::vector<light::item::Light> & lights)
          :light::DecrementPower(lights)
          , JsDriverDpaCommandSolver(iJsRenderService)
        {}

        virtual ~DecrementPower() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.light.DecrementPower";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          jsdriver::SetPower::parsePrevVals(m_prevVals, v);
        }
      };
      typedef std::unique_ptr<DecrementPower> DecrementPowerPtr;

    } //namespace jsdriver
  } //namespace light
} //namespace iqrf
