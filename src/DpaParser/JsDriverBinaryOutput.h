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
#include "BinaryOutput.h"
#include "JsonUtils.h"

namespace iqrf
{
  namespace binaryoutput
  {
    namespace jsdriver {
      ////////////////
      class Enumerate : public binaryoutput::Enumerate, public JsDriverDpaCommandSolver
      {
      public:
        Enumerate(IJsRenderService* iJsRenderService, uint16_t nadr)
          :JsDriverDpaCommandSolver(iJsRenderService, nadr)
        {}

        virtual ~Enumerate() {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.binaryoutput.Enumerate";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          m_outputsNum = jutils::getMemberAs<int>("binOuts", v);
        }
      };
      typedef std::unique_ptr<Enumerate> EnumeratePtr;

    } //namespace jsdriver
  } //namespace binaryoutput
} //namespace iqrf
