/**
 * Copyright 2015-2017 MICRORISC s.r.o.
 * Copyright 2017 IQRF Tech s.r.o.
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

#include "DpaMessage.h"
#include <string>

namespace iqrf {

  class Prf
  {
  public:
    Prf() = delete;

    Prf(uint8_t pNum)
    {
      DpaMessage::DpaPacket_t& packet = m_request.DpaPacket();
      packet.DpaRequestPacket_t.NADR = 0;
      packet.DpaRequestPacket_t.PNUM = pNum;
      packet.DpaRequestPacket_t.PCMD = 0;
      packet.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      m_request.SetLength(sizeof(TDpaIFaceHeader));
    }

    Prf(uint8_t pNum, uint8_t command, uint16_t address)
    {
      DpaMessage::DpaPacket_t& packet = m_request.DpaPacket();
      packet.DpaRequestPacket_t.NADR = address;
      packet.DpaRequestPacket_t.PNUM = pNum;
      packet.DpaRequestPacket_t.PCMD = command;
      packet.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
      m_request.SetLength(sizeof(TDpaIFaceHeader));
    }

    virtual ~Prf() {};

    virtual void parseResponse(const DpaMessage& response) = 0;

    uint16_t getAddress() const
    {
      return m_request.DpaPacket().DpaRequestPacket_t.NADR;
    }

    void setAddress(uint16_t address)
    {
      m_request.DpaPacket().DpaRequestPacket_t.NADR = address;
    }

    uint16_t getHwpid() const
    {
      return m_request.DpaPacket().DpaRequestPacket_t.HWPID;
    }

    void setHwpid(uint16_t hwpid)
    {
      m_request.DpaPacket().DpaRequestPacket_t.HWPID = hwpid;
    }

    uint8_t getPcmd() const
    {
      return m_request.DpaPacket().DpaRequestPacket_t.PCMD;
    }

    void setPcmd(uint8_t command)
    {
      m_request.DpaPacket().DpaRequestPacket_t.PCMD = command;
    }


    const DpaMessage& getDpaRequest() const { return m_request; }

  protected:
    DpaMessage m_request;
  };
}
