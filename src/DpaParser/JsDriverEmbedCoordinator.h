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

        std::string requestParameter() const override
        {
          return "{}";
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
