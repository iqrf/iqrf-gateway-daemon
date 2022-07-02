/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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

#include <vector>
#include <sstream>
#include <iomanip>

namespace iqrf
{
  namespace embed
  {
    namespace os
    {
      ////////////////
      class Read
      {
      protected:
        unsigned m_mid = 0;
        int m_osVersion = 0;
        int m_trMcuType = 0;
        int m_osBuild = 0;
        int m_rssi = 0;
        double m_supplyVoltage = 0;
        int m_flags = 0;
        int m_slotLimits = 0;

        //member read for dpa > 3.03 only
        bool m_is303Compliant = false;
        std::vector<uint8_t> m_ibk;

        //enum members
        //members read for dpa > 4.10 only
        bool m_is410Compliant = false;
        int m_dpaVer = 0;
        int m_perNr = 0;
        std::set<int> m_embedPer;
        int m_hwpidValEnum = 0;
        int m_hwpidVer = 0;
        int m_flagsEnum = 0;
        std::set<int> m_userPer;

        Read()
        {}

      public:
        virtual ~Read() {}

        unsigned getMid() const { return m_mid; }
        int getOsVersion() const { return m_osVersion; }
        int getTrMcuType() const { return m_trMcuType; }
        int getOsBuild() const { return m_osBuild; }
        int getRssi() const { return m_rssi; }
        double getSupplyVoltage() const { return m_supplyVoltage; }
        int getFlags() const { return m_flags; }
        int getSlotLimits() const { return m_slotLimits; }

        bool is303Compliant() const { return m_is303Compliant; }
        bool is410Compliant() const { return m_is410Compliant; }

        const std::vector<uint8_t> & getIbk() const { return m_ibk; }

        int getDpaVer() const { return m_dpaVer & 0x3fff; }
        int getPerNr() const { return m_perNr; }
        const std::set<int> & getEmbedPer() const { return m_embedPer; }
        int getHwpidValFromOs() const { return m_hwpidValEnum; }
        int getHwpidVer() const { return m_hwpidVer; }
        int getFlagsEnum() const { return m_flagsEnum; }
        const std::set<int> & getUserPer() const { return m_userPer; }

        std::string getMidAsString() const
        {
          std::ostringstream moduleId;
          moduleId.fill( '0' );
          moduleId << std::hex << std::uppercase <<
            std::setw( 2 ) << (int)( 0xFF & ( m_mid >> 24 ) ) <<
            std::setw( 2 ) << (int)( 0xFF & ( m_mid >> 16 ) ) <<
            std::setw( 2 ) << (int)( 0xFF & ( m_mid >> 8 ) ) <<
            std::setw( 2 ) << (int)( 0xFF & ( m_mid ) );
          return moduleId.str();
        }

        static std::string getOsVersionAsString( int osVersion )
        {
          std::ostringstream os;
          os << std::hex << (int)( osVersion >> 4 ) << '.';
          os.fill( '0' );
          os << std::setw( 2 ) << (int)( osVersion & 0xf ) << 'D';
          return os.str();
        }

        std::string getOsVersionAsString() const
        {
          return getOsVersionAsString( m_osVersion );
        }

        std::string getTrTypeAsString() const
        {
          std::string trTypeStr = "(DC)TR-";
          switch ( m_trMcuType >> 4 ) {
            case 2: trTypeStr += "72Dx";
              break;
            case 4: trTypeStr += "78Dx";
              break;
            case 11: trTypeStr += "76Dx";
              break;
            case 12: trTypeStr += "77Dx";
              break;
            case 13: trTypeStr += "75Dx";
              break;
            default: trTypeStr += "???";
              break;
          }
          return trTypeStr;
        }

        bool isFccCertified() const { return ( m_trMcuType & 0x08 ) != 0; }

        std::string getTrMcuTypeAsString() const { return ( ( m_trMcuType & 0x07 ) == 0x04 ) ? "PIC16LF1938" : "UNKNOWN"; }

        static std::string getOsBuildAsString( int osBuild )
        {
          std::ostringstream os;
          os.fill( '0' ); os.width( 4 );
          os << std::hex << std::uppercase << osBuild;
          return os.str();
        }

        std::string getOsBuildAsString() const
        {
          return getOsBuildAsString( m_osBuild );
        }

        int getRssiComputed() const { return m_rssi - 130; }

        std::string getRssiAsString() const
        {
          std::ostringstream os;
          os << getRssiComputed() << " dBm";
          return os.str();
        }

        std::string getSupplyVoltageAsString() const
        {
          std::ostringstream os;
          os << std::setprecision( 2 ) << getSupplyVoltage() << " V";
          return os.str();
        }

        bool isInsufficientOsBuild() const { return ( m_flags & 0x01 ) != 0; }
        bool getInterface() const { return ( m_flags & 0x02 ) != 0; }
        std::string getInterfaceAsString() const { return ( m_flags & 0x02 ) != 0 ? "UART" : "SPI"; }
        bool isDpaHandlerDetected() const { return ( m_flags & 0x04 ) != 0; }
        bool isDpaHandlerNotDetectedButEnabled() const { return ( m_flags & 0x08 ) != 0; }
        bool isNoInterfaceSupported() const { return ( m_flags & 0x10 ) != 0; }
        bool isIqrfOsChanges() const { return ( m_flags & 0x20 ) != 0; }
        bool isFrcAggregationEnabled() const { return (m_flags & 0x40) != 0; }

        int getShortestTimeSlot() const
        {
          int s = ( ( m_slotLimits & 0x0f ) + 3 ) * 10;
          return  s;
        }

        std::string getShortestTimeSlotAsString() const
        {
          std::ostringstream os;
          os << getShortestTimeSlot() << " ms";
          return os.str();
        }

        int getLongestTimeSlot() const
        {
          int s = ( ( ( m_slotLimits >> 0x04 ) & 0x0f ) + 3 ) * 10;
          return  s;
        }

        std::string getLongestTimeSlotAsString() const
        {
          std::ostringstream os;
          os << getLongestTimeSlot() << " ms";
          return os.str();
        }

        //enum functions for dpa > 4.10 only
        static std::string getDpaVerAsString( int dpaVer )
        {
          std::ostringstream os;
          os << std::hex << std::setw( 1 ) << ( ( dpaVer & 0x3fff ) >> 8 ) << '.' << std::setw( 2 ) << ( dpaVer & 0xff );
          return os.str();
        }

        std::string getDpaVerAsString() const
        {
          return getDpaVerAsString( m_dpaVer );
        }

        static std::string getDpaVerAsHexaString( int dpaVer )
        {
          std::ostringstream os;
          os.fill( '0' );
          os << std::hex << std::setw( 4 ) << dpaVer;
          return os.str();
        }

        std::string getDpaVerAsHexaString() const
        {
          return getDpaVerAsHexaString( m_dpaVer );
        }

        bool getDemoFlag() const { return ( m_dpaVer & 0x8000 ) != 0; }
        int getModeStd() const { return ( m_flagsEnum & 1 ) ? 1 : 0; }
        bool isModeStd() const { return ( m_flagsEnum & 1 ) != 0; }
        bool isModeLp() const { return ( m_flagsEnum & 1 ) == 0; }
        int getStdAndLpSupport() const { return ( m_flagsEnum & 0b100 ) ? 1 : 0; }
        bool isStdAndLpSupport() const { return ( m_flagsEnum & 0b100 ) != 0; }
      };

      typedef std::unique_ptr<Read> ReadPtr;

      ////////////////
      class ReadCfg
      {
      protected:
        unsigned m_checkum = 0;
        std::vector<uint8_t> m_configuration;
        int m_rfpgm = 0;
        int m_undocumented = 0;

        ReadCfg()
        {}

      public:
        virtual ~ReadCfg() {}

        unsigned getCheckum() const { return m_checkum; }
        std::vector<uint8_t> getConfiguration() const { return m_configuration; }
        int getRfpgm() const { return m_rfpgm; }
        int getUndocumented() const { return m_undocumented; }
      };

      ////////////////
      class Restart
      {
      protected:
        Restart()
        {}
      public:
        virtual ~Restart() {}
      };

      //TODO
      /*
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
      */


    } //namespace os
  } //namespace embed
} //namespace iqrf
