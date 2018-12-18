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

#include "PrfOs.h"
#include "Trace.h"
#include <sstream>
#include <iomanip>

namespace iqrf {

  //using namespace std::chrono;

  PrfOs::PrfOs(uint16_t address)
    : Prf(PNUM_OS, 0, address)
  {
    setReadCmd();
  }

  PrfOs::~PrfOs()
  {
  }

  void PrfOs::parseResponse(const DpaMessage& response)
  {
    switch (getCmd()) {

    case Cmd::READ: {
      TPerOSRead_Response resp = response.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSRead_Response;

      {
        std::ostringstream os;
        os.fill('0');

        os << std::hex <<
          std::setw(2) << (int)resp.MID[3] <<
          std::setw(2) << (int)resp.MID[2] <<
          std::setw(2) << (int)resp.MID[1] <<
          std::setw(2) << (int)resp.MID[0];

        m_moduleId = os.str();
      }

      {
        std::ostringstream os;
        
        os << std::hex <<
          (int)(resp.OsVersion >> 4) << '.';

        os.fill('0');
        os << std::setw(2) << (int)(resp.OsVersion & 0xf) << 'D';

        m_osVersion = os.str();
      }

      m_trType = (resp.MID[3] & 0x80) ? "DCTR-" : "TR-";

      switch (resp.McuType >> 4) {
      case 0: m_trType += "52D"; break;
      case 1: m_trType += "58D-RJ"; break;
      case 2: m_trType += "72D"; break;
      case 3: m_trType += "53D"; break;
      case 8: m_trType += "54D"; break;
      case 9: m_trType += "55D"; break;
      case 10: m_trType += "56D"; break;
      case 11: m_trType += "76D"; break;
      default: m_trType += "???";
      }

      m_fcc = resp.McuType & 0x8;

      int pic = resp.McuType & 0x7;
      switch (pic) {
      case 3: m_mcuType = "PIC16F886"; break;
      case 4: m_mcuType = "PIC16F1938"; break;
      default: m_mcuType = "UNKNOWN";
      }

      {
        std::ostringstream os;
        os.fill('0');

        os << std::hex << std::uppercase << std::setw(4) << (int)resp.OsBuild;
        m_osBuild = os.str();
      }
    }
    break;

    default:;
    }
  }

  void PrfOs::setSleepCmd(const std::chrono::seconds& sec, uint8_t ctrl)
  {
    using namespace std::chrono;

    setCmd(Cmd::SLEEP);

    ctrl &= 0x0F; //reset milis flag

    milis2097 ms2097 = duration_cast<milis2097>(sec);
    uint16_t tm = (uint16_t)ms2097.count();

    m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.PerOSSleep_Request.Time = tm;
    m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.PerOSSleep_Request.Control = ctrl;
    m_request.SetLength(sizeof(TDpaIFaceHeader) + 3);
  }

  void PrfOs::setSleepCmd(const std::chrono::milliseconds& milis, uint8_t ctrl)
  {
    using namespace std::chrono;

    setCmd(Cmd::SLEEP);

    ctrl &= 0x0F; //reset other flags
    ctrl |= 0x10; //set milis flags

    micros32768 mc32768 = duration_cast<micros32768>(milis);
    uint16_t tm = (uint16_t)mc32768.count();

    m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.PerOSSleep_Request.Time = tm;
    m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.PerOSSleep_Request.Control = ctrl;
    m_request.SetLength(sizeof(TDpaIFaceHeader) + 3);
  }

  void PrfOs::setSleepCmd()
  {
    setCmd(Cmd::SLEEP);

    m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.PerOSSleep_Request.Time = 0;
    m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.PerOSSleep_Request.Control = (uint8_t)TimeControl::RUN_CALIB;
  }

  void PrfOs::setResetCmd()
  {
    setCmd(Cmd::RESET);
    m_request.SetLength(sizeof(TDpaIFaceHeader));
  }

  void PrfOs::setRestartCmd()
  {
    setCmd(Cmd::RESTART);
    m_request.SetLength(sizeof(TDpaIFaceHeader));
  }

  void PrfOs::setReadCmd()
  {
    setCmd(Cmd::READ);
    m_request.SetLength(sizeof(TDpaIFaceHeader));
  }

  PrfOs::Cmd PrfOs::getCmd() const
  {
    return m_cmd;
  }

  void PrfOs::setCmd(PrfOs::Cmd cmd)
  {
    m_cmd = cmd;
    setPcmd((uint8_t)m_cmd);
  }

}