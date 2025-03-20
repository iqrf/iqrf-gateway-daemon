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
