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
