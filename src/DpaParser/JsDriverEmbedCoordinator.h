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
        JsDriverBondedDevices()
          :JsDriverDpaCommandSolver(0)
        {
        }

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
        JsDriverDiscoveredDevices()
          :JsDriverDpaCommandSolver(0)
        {
        }

        std::string functionName() const override
        {
          return "iqrf.embed.coordinator.DiscoveredDevices";
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
