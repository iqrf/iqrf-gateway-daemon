/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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
#include "EmbedOS.h"

namespace iqrf
{
  namespace embed
  {
    namespace os
    {
      ////////////////
      class RawDpaRead : public Read, public DpaCommandSolver
      {
      public:
        RawDpaRead(uint16_t nadr)
          :DpaCommandSolver(nadr, PNUM_OS, CMD_OS_READ)
        {}

        virtual ~RawDpaRead() {}

      protected:
        void encodeRequest(DpaMessage & dpaRequest) override
        {
          (void)dpaRequest; //silence -Wunused-parameter
        }

        void parseResponse(const DpaMessage & dpaResponse) override
        {

          TPerOSRead_Response resp = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSRead_Response;

          m_mid = (unsigned)resp.MID[0] + ((unsigned)resp.MID[1] << 8) + ((unsigned)resp.MID[2] << 16) + ((unsigned)resp.MID[3] << 24);
          m_osVersion = (int)resp.OsVersion;
          m_trMcuType = (uint8_t)resp.McuType;
          m_trSeries = m_trMcuType >> 4;
          m_mcuType = m_trMcuType & 0x07;
          m_osBuild = (int)resp.OsBuild;
          m_rssi = (int)resp.Rssi;
          m_supplyVoltage = 261.12 / (127 - (int)resp.SupplyVoltage);
          m_flags = (int)resp.Flags;
          m_slotLimits = (int)resp.SlotLimits;

          // False at DPA < 3.03
          if (m_rdata.size() > 12 + 16) {
            m_ibk = std::vector<uint8_t>(resp.IBK, resp.IBK + 16);
            m_is303Compliant = true;
          }

          // Only in DSM the next condition would be false or at DPA < 4.10
          if (m_rdata.size() > 28 + 11 ) {
            m_dpaVer = (int)resp.DpaVersion;
            m_perNr = (int)resp.UserPerNr;
            m_embedPer = HexStringConversion::bitmapToIndexes(resp.EmbeddedPers, 0, 3, 0);
            m_hwpidValEnum = (int)resp.HWPID;
            m_hwpidVer = (int)resp.HWPIDver;
            m_flags = (int)resp.Flags;
            m_userPer = HexStringConversion::bitmapToIndexes(resp.UserPer, 0, 11, 0x20);
            m_is410Compliant = true;
          }
        }
      };
      typedef std::unique_ptr<RawDpaRead> RawDpaReadPtr;

      ////////////////
      class RawDpaReadCfg : public ReadCfg, public DpaCommandSolver
      {
      public:
        RawDpaReadCfg(uint16_t nadr)
          :DpaCommandSolver(nadr, PNUM_OS, CMD_OS_READ_CFG)
        {}

        virtual ~RawDpaReadCfg() {}

      protected:
        void encodeRequest(DpaMessage & dpaRequest) override
        {
          (void)dpaRequest; //silence -Wunused-parameter
        }

        void parseResponse(const DpaMessage & dpaResponse) override
        {
          TPerOSReadCfg_Response resp = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSReadCfg_Response;

          m_checkum = (unsigned)resp.Checksum;
          m_configuration = std::vector<uint8_t>(resp.Configuration, resp.Configuration + 31);
          m_rfpgm = (int)resp.RFPGM;
          m_undocumented = (int)resp.Undocumented[0];
        }
      };
      typedef std::unique_ptr<RawDpaReadCfg> RawDpaReadCfgPtr;

      ////////////////
      class RawDpaRestart : public ReadCfg, public DpaCommandSolver
      {
      public:
        RawDpaRestart(uint16_t nadr)
          :DpaCommandSolver(nadr, PNUM_OS, CMD_OS_RESTART)
        {}

        virtual ~RawDpaRestart() {}

      protected:
        void encodeRequest(DpaMessage & dpaRequest) override
        {
          (void)dpaRequest; //silence -Wunused-parameter
        }

        void parseResponse(const DpaMessage & dpaResponse) override
        {
          (void)dpaResponse; //silence -Wunused-parameter
        }
      };
      typedef std::unique_ptr<RawDpaRestart> RawDpaRestartPtr;

    } //namespace os
  } //namespace embed
} //namespace iqrf
