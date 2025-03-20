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
