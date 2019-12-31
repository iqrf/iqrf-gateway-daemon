#pragma once

#include "DpaCommandSolver.h"
#include "EmbedEEEPROM.h"

namespace iqrf
{
  namespace embed
  {
    namespace eeeprom
    {
      namespace rawdpa
      {
        ////////////////
        class Read : public eeeprom::Read, public DpaCommandSolver
        {
        public:
          Read(uint16_t nadr, uint16_t address, uint8_t len)
            :eeeprom::Read(address, len)
            , DpaCommandSolver(nadr, PNUM_EEEPROM, CMD_EEEPROM_XREAD)
          {}

          virtual ~Read()
          {}

        protected:
          void encodeRequest(DpaMessage & dpaRequest) override
          {
            dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.XMemoryRequest.Address = m_address;
            dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.XMemoryRequest.ReadWrite.Read.Length = m_len;
            dpaRequest.SetLength(getRequestHeaderLen() + sizeof(uint16_t) + sizeof(uint8_t));
          }

          void parseResponse(const DpaMessage & dpaResponse) override
          {
            if (dpaResponse.GetLength() < (int)getResponseHeaderLen() + m_len) {
              THROW_EXC_TRC_WAR(std::logic_error, "Unexpected response length");
            }
            m_pdata.assign(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData,
              dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData + m_len);
          }
        };
        typedef std::unique_ptr<Read> ReadPtr;

      } //namespace rawdpa
    } //namespace eeeprom
  } //namespace embed
} //namespace iqrf
