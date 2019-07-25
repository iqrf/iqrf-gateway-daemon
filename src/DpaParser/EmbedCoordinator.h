#pragma once

#include <set>

namespace iqrf
{
  namespace embed
  {
    namespace coordinator
    {
      ////////////////
      class BondedDevices
      {
      protected:
        std::set<int> m_bondedDevices;

        BondedDevices()
        {}

      public:
        virtual ~BondedDevices()
        {}

        const std::set<int> & getBondedDevices() const { return m_bondedDevices; }

        bool isBonded(uint16_t nadr)
        {
          return m_bondedDevices.find(nadr) != m_bondedDevices.end();
        }
      };

      ////////////////
      class DiscoveredDevices
      {
      protected:
        std::set<int> m_discoveredDevices;

      public:
        DiscoveredDevices()
        {}

        virtual ~DiscoveredDevices()
        {}

        const std::set<int> & getDiscoveredDevices() const { return m_discoveredDevices; }

        // get more detailed data parsing
        bool isDiscovered(uint16_t nadr)
        {
          return m_discoveredDevices.find(nadr) != m_discoveredDevices.end();
        }
      };

    } //namespace coordinator
  } //namespace embed
} //namespace iqrf
