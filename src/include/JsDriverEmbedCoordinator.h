#pragma once

#include "JsDriverRequest.h"
#include "JsonUtils.h"
#include <vector>
#include <sstream>
#include <iomanip>

namespace iqrf
{
  namespace embed
  {
    namespace coordinator
    {
      ////////////////
      class BondedDevices : public JsDriverRequest
      {
      private:
        std::vector<int> m_bondedDevices;

      public:
        BondedDevices()
          :JsDriverRequest(0)
        {
        }

        std::string functionName() const override
        {
          return "iqrf.embed.coordinator.BondedDevices";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;
          m_bondedDevices = jutils::getMemberAsVector<int>("bondedDevices", v);
        }

        // get data as returned from driver
        const std::vector<int> & getBondedDevices() const { return m_bondedDevices; }

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
      class DiscoveredDevices : public JsDriverRequest
      {
      private:
        std::vector<int> m_discoveredDevices;

      public:
        DiscoveredDevices()
          :JsDriverRequest(0)
        {
        }

        std::string functionName() const override
        {
          return "iqrf.embed.coordinator.DiscoveredDevices";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;
          m_discoveredDevices = jutils::getMemberAsVector<int>("discoveredDevices", v);
        }

        // get data as returned from driver
        const std::vector<int> & getDiscoveredDevices() const { return m_discoveredDevices; }

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
