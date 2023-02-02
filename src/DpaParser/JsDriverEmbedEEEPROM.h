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
