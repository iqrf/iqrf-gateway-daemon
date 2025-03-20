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
#include "EmbedEEEPROM.h"
#include "JsonUtils.h"

namespace iqrf
{
  namespace embed
  {
    namespace eeeprom
    {
      ////////////////
      class JsDriverRead : public Read, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverRead(IJsRenderService* iJsRenderService, uint16_t nadr, uint16_t address, uint8_t len)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
          ,Read(address, len)
        {}

        virtual ~JsDriverRead()
        {}

        std::string functionName() const override
        {
          return "iqrf.embed.eeeprom.Read";
        }

        void requestParameter(rapidjson::Document& par) const override
        {
          using namespace rapidjson;
          Pointer("/address").Set(par, (int)m_address);
          Pointer("/len").Set(par, (int)m_len);
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          m_pdata = jutils::getMemberAsVector<int>("pData", v);
        }

      };

    } //namespace eeeprom
  } //namespace embed
} //namespace iqrf
