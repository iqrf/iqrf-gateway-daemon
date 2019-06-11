#pragma once

#include "RawDpaCommandSolver.h"
#include "EmbedCoordinator.h"

namespace iqrf
{
  namespace embed
  {
    namespace coordinator
    {
      std::set<int> bitmapToIndexes(const uint8_t* bitmap, int indexFrom, int indexTo, int offset)
      {
        std::set<int> retval;

        for (int index = indexFrom; index <= indexTo; index++) {
          unsigned bitmapByte = bitmap[index];
          if (0 == bitmapByte) {
            offset += 8;
          }
          else {
            for (unsigned bitMask = 0x01; bitMask != 0x100; bitMask <<= 1) {
              if ((bitmapByte & bitMask) != 0) {
                retval.insert(offset);
              }
              offset++;
            }
          }
        }
        return retval;
      }

      ////////////////
      class RawDpaBondedDevices : public BondedDevices, public RawDpaCommandSolver
      {
      public:
        RawDpaBondedDevices()
          :RawDpaCommandSolver(COORDINATOR_ADDRESS, PNUM_COORDINATOR, CMD_COORDINATOR_BONDED_DEVICES)
        {
        }

        void parseResponse() override
        {
          const uint8_t* bonded = m_dpaTransactionResult2->getResponse().DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
          m_bondedDevices = bitmapToIndexes(bonded, 0, 29, 0);
        }

      };

      ////////////////
      class RawDpaDiscoveredDevices : public DiscoveredDevices, public RawDpaCommandSolver
      {
      public:
        RawDpaDiscoveredDevices()
          :RawDpaCommandSolver(COORDINATOR_ADDRESS, PNUM_COORDINATOR, CMD_COORDINATOR_DISCOVERED_DEVICES)
        {
        }

        void parseResponse() override
        {
          const uint8_t* discovered = m_dpaTransactionResult2->getResponse().DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData;
          m_discoveredDevices = bitmapToIndexes(discovered, 0, 29, 0);
        }

      };

    } //namespace coordinator
  } //namespace embed
} //namespace iqrf
