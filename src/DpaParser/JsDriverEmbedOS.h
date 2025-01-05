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
#include "EmbedOS.h"
#include "JsonUtils.h"
#include <vector>
#include <sstream>
#include <iomanip>

namespace iqrf
{
  namespace embed
  {
    namespace os
    {
      ////////////////
      class JsDriverRead : public Read, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverRead(IJsRenderService* iJsRenderService, uint16_t nadr)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
        {}

        virtual ~JsDriverRead()
        {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.os.Read";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;
          m_mid = jutils::getMemberAs<unsigned>("mid", v);
          m_osVersion = jutils::getMemberAs<int>("osVersion", v);
          m_trMcuType = jutils::getMemberAs<int>("trMcuType", v);
          m_osBuild = jutils::getMemberAs<int>("osBuild", v);
          m_rssi = jutils::getMemberAs<int>("rssi", v);
          m_supplyVoltage = jutils::getMemberAs<double>("supplyVoltage", v);
          m_flags = jutils::getMemberAs<int>("flags", v);

          //TODO check 303
          if (Pointer("/dpaVer").Get(v)) {
            auto vect = jutils::getPossibleMemberAsVector<int>("ibk", v);
            m_ibk = std::vector<uint8_t>(vect.begin(), vect.end());
            m_is303Compliant = true;
          }

          //TODO check 410
          if (Pointer("/dpaVer").Get(v)) {
            m_dpaVer = jutils::getPossibleMemberAs<int>("dpaVer", v, m_dpaVer);
            m_perNr = jutils::getPossibleMemberAs<int>("perNr", v, m_perNr);
            {
              auto vect = jutils::getPossibleMemberAsVector<int>("embeddedPers", v);
              m_embedPer = std::set<int>(vect.begin(), vect.end());
            }
            m_hwpidValEnum = jutils::getPossibleMemberAs<int>("hwpid", v, m_hwpidValEnum);
            m_hwpidVer = jutils::getPossibleMemberAs<int>("hwpidVer", v, m_hwpidVer);
            m_flagsEnum = jutils::getPossibleMemberAs<int>("flagsEnum", v, m_flagsEnum);
            {
              auto vect = jutils::getPossibleMemberAsVector<int>("userPer", v);
              m_userPer = std::set<int>(vect.begin(), vect.end());
            }
            m_is410Compliant = true;
          }

        }
      };

      ////////////////
      class JsDriverReadCfg : public ReadCfg, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverReadCfg(IJsRenderService* iJsRenderService, uint16_t nadr)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
        {}

        virtual ~JsDriverReadCfg()
        {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.os.ReadCfg";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          m_checkum = jutils::getMemberAs<int>("checksum", v);
          std::vector<int> arr;
          arr = jutils::getPossibleMemberAsVector<int>("configuration", v, arr);
          m_configuration = std::vector<uint8_t>(arr.begin(), arr.end());
          m_rfpgm = jutils::getMemberAs<int>("rfpgm", v);
          m_undocumented = jutils::getMemberAs<int>("undocumented", v);
        }

      };

      ////////////////
      class JsDriverRestart : public ReadCfg, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverRestart(IJsRenderService* iJsRenderService, uint16_t nadr)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
        {}

        virtual ~JsDriverRestart()
        {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.os.Restart";
        }

        void parseResponse(const rapidjson::Value& v) override
        {}

      };

    } //namespace os
  } //namespace embed
} //namespace iqrf
