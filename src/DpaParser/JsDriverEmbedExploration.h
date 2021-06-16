/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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
#include "EmbedExplore.h"
#include "JsonUtils.h"
#include <sstream>
#include <iomanip>

namespace iqrf
{
  namespace embed
  {
    namespace explore
    {
      ////////////////
      class JsDriverEnumerate : public Enumerate, public JsDriverDpaCommandSolver
      {

      public:
        JsDriverEnumerate(IJsRenderService* iJsRenderService, uint16_t nadr)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
        {}

        virtual ~JsDriverEnumerate()
        {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.explore.Enumerate";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          m_dpaVer = jutils::getMemberAs<int>("dpaVer", v);
          m_perNr = jutils::getMemberAs<int>("perNr", v);
          {
            auto vect = jutils::getMemberAsVector<int>("embeddedPers", v);
            m_embedPer = std::set<int>(vect.begin(), vect.end());
          }
          m_hwpid = jutils::getMemberAs<int>("hwpid", v);
          m_hwpidVer = jutils::getMemberAs<int>("hwpidVer", v);
          m_flags = jutils::getMemberAs<int>("flags", v);
          {
            auto vect = jutils::getMemberAsVector<int>("userPer", v);
            m_userPer = std::set<int>(vect.begin(), vect.end());
          }
        }

      };

      ////////////////
      class JsDriverPeripheralInformation : public PeripheralInformation, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverPeripheralInformation(IJsRenderService* iJsRenderService, uint16_t nadr, int per)
          :PeripheralInformation(per)
          , JsDriverDpaCommandSolver(iJsRenderService, nadr)
        {}

        virtual ~JsDriverPeripheralInformation()
        {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.explore.PeripheralInformation";
        }

        void requestParameter(rapidjson::Document& par) const override
        {
          using namespace rapidjson;
          Pointer("/per").Set(par, (int)m_per);
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;

          m_perTe = jutils::getMemberAs<int>("perTe", v);
          m_perT = jutils::getMemberAs<int>("perT", v);
          m_par1 = jutils::getMemberAs<int>("par1", v);
          m_par2 = jutils::getMemberAs<int>("par2", v);
        }
      };
    } //namespace explore
  } //namespace embed
} //namespace iqrf
