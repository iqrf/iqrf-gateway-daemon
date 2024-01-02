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
