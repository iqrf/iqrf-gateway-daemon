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

#include "PrfEnum.h"
#include "Trace.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace iqrf {

  PrfEnum::PrfEnum(uint16_t address)
    : Prf(PNUM_OS, 0, address)
  {
    setEnumerationCmd();
  }

  PrfEnum::~PrfEnum()
  {
  }

  bool PrfEnum::parseCoordinatorResetAsyncResponse(const DpaMessage& response)
  {
    const DpaMessage::DpaPacket_t& p = response.DpaPacket();
    if (p.DpaResponsePacket_t.NADR == 0 &&
      p.DpaResponsePacket_t.PNUM == PNUM_ENUMERATION &&
      p.DpaResponsePacket_t.PCMD == CMD_GET_PER_INFO)
    {
      parseResponse(response);
      return true;
    }
    return false;
  }

  void PrfEnum::parseResponse(const DpaMessage& response)
  {
    switch (getCmd()) {

    case Cmd::INFO: {
      m_resp = response.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer;
      {
        //DPA version
        m_dpaVerWord = m_resp.DpaVersion;
        {
          std::ostringstream os;
          os.fill('0');
          os << std::hex <<
            std::setw(2) << ((m_resp.DpaVersion & 0xefff) >> 8) << '.' << std::setw(2) << (m_resp.DpaVersion & 0xff);
          m_dpaVer = os.str();
        }
        {
          std::ostringstream os;
          os.fill('0');
          os << std::hex <<
            std::setw(2) << ((m_resp.DpaVersion & 0xefff) >> 8) << std::setw(2) << (m_resp.DpaVersion & 0xff);
          m_dpaVerWordAsStr = os.str();
        }
        m_demo = 0 != (m_resp.DpaVersion & 0x8000);

        std::string buf(m_dpaVer);
        std::replace(buf.begin(), buf.end(), '.', ' ');
        std::istringstream is(buf);
        is >> m_dpaVerMajor >> m_dpaVerMinor;

        //RF Mode support
        m_stdModeSupport = 0 != (m_resp.Flags & 1);
        m_lpModeSupport = 0 != (m_resp.Flags & 2);
        m_lpModeRunning = 0 != (m_resp.Flags & 4);

      }
      

    }
    break;

    default:;
    }
  }

  void PrfEnum::setEnumerationCmd()
  {
    setCmd(Cmd::INFO);
    m_request.SetLength(sizeof(TDpaIFaceHeader));
  }

  PrfEnum::Cmd PrfEnum::getCmd() const
  {
    return m_cmd;
  }

  void PrfEnum::setCmd(PrfEnum::Cmd cmd)
  {
    m_cmd = cmd;
    setPcmd((uint8_t)m_cmd);
  }

}