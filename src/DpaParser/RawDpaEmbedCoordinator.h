/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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
