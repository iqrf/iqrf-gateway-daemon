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
#include "EmbedExplore.h"
#include "JsonUtils.h"
#include <set>
#include <sstream>
#include <iomanip>

namespace iqrf
{
  namespace embed
  {
    namespace explore
    {
      ////////////////
      class RawDpaEnumerate : public Enumerate, public DpaCommandSolver
      {
      public:
        RawDpaEnumerate(uint16_t nadr)
          :DpaCommandSolver(nadr, PNUM_ENUMERATION, CMD_GET_PER_INFO)
        {}

        virtual ~RawDpaEnumerate()
        {
        }

      protected:
        void encodeRequest(DpaMessage & dpaRequest) override
        {
          (void)dpaRequest; //silence -Wunused-parameter
        }

        void parseResponse(const DpaMessage & dpaResponse) override
        {
          const TEnumPeripheralsAnswer & resp = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer;

          m_dpaVer = (int)resp.DpaVersion;
          m_perNr = (int)resp.UserPerNr;
          m_embedPer = HexStringConversion::bitmapToIndexes(resp.EmbeddedPers, 0, 3, 0);
          m_hwpidEnm = (int)resp.HWPID;
          m_hwpid = (int)resp.HWPID;
          m_hwpidVer = (int)resp.HWPIDver;
          m_flags = (int)resp.Flags;
          m_userPer = HexStringConversion::bitmapToIndexes(getRdata().data(), 12, (int)getRdata().size() - 1, 0x20);
        }
      };
      typedef std::unique_ptr<RawDpaEnumerate> RawDpaEnumeratePtr;

      ////////////////
      class RawDpaPeripheralInformation : public PeripheralInformation, public DpaCommandSolver
      {
      public:
        RawDpaPeripheralInformation(uint16_t nadr, int per)
          :PeripheralInformation(per)
          , DpaCommandSolver(nadr, (uint8_t)per, CMD_GET_PER_INFO)
        {}

        virtual ~RawDpaPeripheralInformation() {}

      protected:
        void encodeRequest(DpaMessage & dpaRequest) override
        {
          dpaRequest.DpaPacket().DpaRequestPacket_t.PNUM = (uint8_t)getPer();
        }

        void parseResponse(const DpaMessage & dpaResponse) override
        {
          TPeripheralInfoAnswer resp = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PeripheralInfoAnswer;
          m_perTe = (int)resp.PerTE;
          m_perT = (int)resp.PerT;
          m_par1 = (int)resp.Par1;
          m_par2 = (int)resp.Par2;
        }

      };
      typedef std::unique_ptr<RawDpaPeripheralInformation> RawDpaPeripheralInformationPtr;

      ////////////////
      class RawDpaMorePeripheralInformation : public MorePeripheralInformation, public DpaCommandSolver
      {
      public:
        RawDpaMorePeripheralInformation(uint16_t nadr, int per)
          :MorePeripheralInformation(per)
          , DpaCommandSolver(nadr, 0xFF, (uint8_t)per)
        {}

        virtual ~RawDpaMorePeripheralInformation() {}

      protected:
        void encodeRequest(DpaMessage & dpaRequest) override
        {
          (void)dpaRequest; //silence -Wunused-parameter
        }

        void parseResponse(const DpaMessage & dpaResponse) override
        {
          (void)dpaResponse; //silence -Wunused-parameter
          const std::vector<uint8_t> & r = getRdata();
          auto len = r.size();
          for (size_t i = 3; i < len; i += 4) {
            m_params.push_back(MorePeripheralInformation::Param(r[i - 3], r[i - 2], r[i - 1], r[i]));
          }
        }

      };
      typedef std::unique_ptr<RawDpaMorePeripheralInformation> RawDpaMorePeripheralInformationPtr;

    } //namespace explore
  } //namespace embed
} //namespace iqrf
