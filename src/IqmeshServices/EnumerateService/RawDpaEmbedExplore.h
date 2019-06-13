#pragma once

#include "JsDriverDpaCommandSolver.h"
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
      class RawDpaEnumerate : public Enumerate, public RawDpaCommandSolver
      {
      public:
        RawDpaEnumerate(uint16_t nadr)
          :RawDpaCommandSolver(nadr, PNUM_ENUMERATION, CMD_GET_PER_INFO)
        {}

        virtual ~RawDpaEnumerate() {}

        void parseResponse() override
        {
          TEnumPeripheralsAnswer resp = m_dpaTransactionResult2->getResponse().DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer;

          m_dpaVer = (int)resp.DpaVersion;
          m_perNr = (int)resp.UserPerNr;
          m_embedPer = bitmapToIndexes(resp.EmbeddedPers, 0, 3, 0);
          m_hwpidVer = (int)resp.HWPIDver;
          m_flags = (int)resp.Flags;
          int len = m_dpaTransactionResult2->getResponse().GetLength() - sizeof(TDpaIFaceHeader);
          m_userPer = bitmapToIndexes(getRdata().data(), 12, getRdata().size()-1, 0x20);
        }

      };
      typedef std::unique_ptr<RawDpaEnumerate> RawDpaEnumeratePtr;

      ////////////////
      class RawDpaPeripheralInformation : public PeripheralInformation, public RawDpaCommandSolver
      {
      public:
        RawDpaPeripheralInformation(uint16_t nadr, int per)
          :PeripheralInformation(per)
          , RawDpaCommandSolver(nadr, (uint8_t)per, CMD_GET_PER_INFO)
        {
        }

        virtual ~RawDpaPeripheralInformation() {}

        DpaMessage encodeRequest() override
        {
          m_request.DpaPacket().DpaRequestPacket_t.PNUM = (uint8_t)getPer();
          return m_request;
        }

        void parseResponse() override
        {
          TPeripheralInfoAnswer resp = m_dpaTransactionResult2->getResponse().DpaPacket().DpaResponsePacket_t.DpaMessage.PeripheralInfoAnswer;
          m_perTe = (int)resp.PerTE;
          m_perT = (int)resp.PerT;
          m_par1 = (int)resp.Par1;
          m_par2 = (int)resp.Par2;
        }

      };
      typedef std::unique_ptr<RawDpaPeripheralInformation> RawDpaPeripheralInformationPtr;

    } //namespace explore
  } //namespace embed
} //namespace iqrf
