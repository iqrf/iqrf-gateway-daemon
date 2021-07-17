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
