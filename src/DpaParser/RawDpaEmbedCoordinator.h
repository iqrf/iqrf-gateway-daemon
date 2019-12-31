#pragma once

#include "DpaCommandSolver.h"
#include "EmbedCoordinator.h"

namespace iqrf
{
  namespace embed
  {
    namespace coordinator
    {
      ////////////////
      class RawDpaBondedDevices : public BondedDevices, public DpaCommandSolver
      {
      public:
        RawDpaBondedDevices()
          :DpaCommandSolver(COORDINATOR_ADDRESS, PNUM_COORDINATOR, CMD_COORDINATOR_BONDED_DEVICES)
        {}

        virtual ~RawDpaBondedDevices()
        {}

      protected:
        void encodeRequest(DpaMessage & dpaRequest) override
        {
          (void)dpaRequest; //silence -Wunused-parameter
        }

        void parseResponse(const DpaMessage & dpaResponse) override
        {
          const uint8_t* bonded = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
          m_bondedDevices = bitmapToIndexes(bonded, 0, 29, 0);
        }
      };
      typedef std::unique_ptr<RawDpaBondedDevices> RawDpaBondedDevicesPtr;

      ////////////////
      class RawDpaDiscoveredDevices : public DiscoveredDevices, public DpaCommandSolver
      {
      public:
        RawDpaDiscoveredDevices()
          :DpaCommandSolver(COORDINATOR_ADDRESS, PNUM_COORDINATOR, CMD_COORDINATOR_DISCOVERED_DEVICES)
        {
        }

        virtual ~RawDpaDiscoveredDevices()
        {}

      protected:
        void encodeRequest(DpaMessage & dpaRequest) override
        {
          (void)dpaRequest; //silence -Wunused-parameter
        }

        void parseResponse(const DpaMessage & dpaResponse) override
        {
          const uint8_t* discovered = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
          m_discoveredDevices = bitmapToIndexes(discovered, 0, 29, 0);
        }
      };
      typedef std::unique_ptr<RawDpaDiscoveredDevices> RawDpaDiscoveredDevicesPtr;

    } //namespace coordinator
  } //namespace embed
} //namespace iqrf
