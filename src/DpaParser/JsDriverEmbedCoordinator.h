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
#include "EmbedCoordinator.h"
#include "JsonUtils.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include <set>

namespace iqrf
{
  namespace embed
  {
    namespace coordinator
    {
      ////////////////
      class JsDriverBondedDevices : public BondedDevices, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverBondedDevices(IJsRenderService* iJsRenderService)
          :JsDriverDpaCommandSolver(iJsRenderService, 0)
        {}

        virtual ~JsDriverBondedDevices()
        {}

      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.coordinator.BondedDevices";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;
          auto vect = jutils::getMemberAsVector<int>("bondedDevices", v);
          m_bondedDevices = std::set<int>(vect.begin(), vect.end());
        }

      };

      ////////////////
      class JsDriverDiscoveredDevices : public DiscoveredDevices, public JsDriverDpaCommandSolver
      {
      public:
        JsDriverDiscoveredDevices(IJsRenderService* iJsRenderService)
          :JsDriverDpaCommandSolver(iJsRenderService, 0)
        {}

        virtual ~JsDriverDiscoveredDevices()
        {}
      
      protected:
        std::string functionName() const override
        {
          return "iqrf.embed.coordinator.DiscoveredDevices";
        }

        std::string requestParameter() const override
        {
          return "{}";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;
          auto vect = jutils::getMemberAsVector<int>("discoveredDevices", v);
          m_discoveredDevices = std::set<int>(vect.begin(), vect.end());
        }

      };

    } //namespace coordinator
  } //namespace embed
} //namespace iqrf
