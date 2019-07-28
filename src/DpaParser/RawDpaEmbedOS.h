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

        DpaMessage encodeRequest() override
        {
          DpaMessage request;
          initRequestHeader(request);
          return request;
        }

      protected:
        void parseResponse(const DpaMessage & dpaResponse) override
        {
          TPerOSRead_Response resp = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSRead_Response;

          m_mid = (unsigned)resp.MID[0] + ((unsigned)resp.MID[1] << 8) + ((unsigned)resp.MID[2] << 16) + ((unsigned)resp.MID[3] << 24);
          m_osVersion = (int)resp.OsVersion;
          m_trMcuType = (int)resp.McuType;
          m_osBuild = (int)resp.OsBuild;
          m_rssi = (int)resp.Rssi;
          m_supplyVoltage = 261.12 / (127 - (int)resp.SupplyVoltage);
          m_flags = (int)resp.Flags;
          m_slotLimits = (int)resp.SlotLimits;
          m_ibk = std::vector<uint8_t>(resp.IBK, resp.IBK + 16);
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

        DpaMessage encodeRequest() override
        {
          DpaMessage request;
          initRequestHeader(request);
          return request;
        }

      protected:
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

        DpaMessage encodeRequest() override
        {
          DpaMessage request;
          initRequestHeader(request);
          return request;
        }

      protected:
        void parseResponse(const DpaMessage & dpaResponse) override
        {
        }
      };
      typedef std::unique_ptr<RawDpaRestart> RawDpaRestartPtr;

    } //namespace os
  } //namespace embed
} //namespace iqrf
