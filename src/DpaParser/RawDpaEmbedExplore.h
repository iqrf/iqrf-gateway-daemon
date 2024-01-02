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
          m_embedPer = bitmapToIndexes(resp.EmbeddedPers, 0, 3, 0);
          m_hwpidEnm = (int)resp.HWPID;
          m_hwpid = (int)resp.HWPID;
          m_hwpidVer = (int)resp.HWPIDver;
          m_flags = (int)resp.Flags;
          m_userPer = bitmapToIndexes(getRdata().data(), 12, (int)getRdata().size() - 1, 0x20);
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
