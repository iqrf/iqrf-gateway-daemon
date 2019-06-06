#pragma once

#include "JsDriverDpaCommandSolver.h"
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
      class BondedDevices : public JsDriverDpaCommandSolver
      {
      private:
        std::set<int> m_bondedDevices;

      public:
        BondedDevices()
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

        // get data as returned from driver
        const std::set<int> & getBondedDevices() const { return m_bondedDevices; }

        // get more detailed data parsing
        bool isBonded(uint16_t nadr)
        {
          for (int i : m_bondedDevices) {
            if (i == (uint16_t)nadr) return true;
          }
          return false;
        }
      };

      ////////////////
      class DiscoveredDevices : public JsDriverDpaCommandSolver
      {
      private:
        std::set<int> m_discoveredDevices;

      public:
        DiscoveredDevices()
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

        // get data as returned from driver
        const std::set<int> & getDiscoveredDevices() const { return m_discoveredDevices; }

        // get more detailed data parsing
        bool isDiscovered(uint16_t nadr)
        {
          for (int i : m_discoveredDevices) {
            if (i == (uint16_t)nadr) return true;
          }
          return false;
        }
      };

    } //namespace coordinator
  } //namespace embed
} //namespace iqrf
