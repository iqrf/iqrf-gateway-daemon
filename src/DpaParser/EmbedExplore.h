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

#include "DpaMessage.h"
#include <set>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>

namespace iqrf
{
  namespace embed
  {
    namespace explore
    {
      ////////////////
      class Enumerate
      {
      protected:
        int m_dpaVer = 0;
        int m_perNr = 0;
        std::set<int> m_embedPer;
        int m_hwpidEnm = 0;
        int m_hwpidVer = 0;
        int m_flags = 0;
        std::set<int> m_userPer;

        Enumerate()
        {}

      public:
        virtual ~Enumerate() {}

        int getDpaVer() const { return m_dpaVer & 0x3fff; }
        int getPerNr() const { return m_perNr; }
        const std::set<int> & getEmbedPer() const { return m_embedPer; }
        int getHwpidEnm() const { return m_hwpidEnm; }
        int getHwpidVer() const { return m_hwpidVer; }
        int getFlags() const { return m_flags; }
        const std::set<int> & getUserPer() const { return m_userPer; }

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

        // TODO obsolete use int getDpaVer()
        int getDpaVerMajor() const { return ( m_dpaVer >> 8 ) & 0xFF; }
        int getDpaVerMinor() const { return m_dpaVer & 0xFF; }
        bool getDemoFlag() const { return ( m_dpaVer & 0x8000 ) != 0; }
        int getModeStd() const { return ( m_flags & 1 ) ? 1 : 0; }
        bool isModeStd() const { return ( m_flags & 1 ) != 0; }
        bool isModeLp() const { return ( m_flags & 1 ) == 0; }
        int getStdAndLpSupport() const { return ( m_flags & 0b100 ) ? 1 : 0; }
        bool isStdAndLpSupport() const { return ( m_flags & 0b100 ) != 0; }
      };
      typedef std::unique_ptr<Enumerate> EnumeratePtr;

      ////////////////
      class PeripheralInformation
      {
      protected:
        //params
        int m_per = 0;

        //response
        int m_perTe = 0;
        int m_perT = 0;
        int m_par1 = 0;
        int m_par2 = 0;

        PeripheralInformation( int per )
          : m_per( per )
        {}

      public:
        virtual ~PeripheralInformation() {}
        int getPer() const { return m_per; }
        int getPerTe() const { return m_perTe; }
        int getPerT() const { return m_perT; }
        int getPar1() const { return m_par1; }
        int getPar2() const { return m_par2; }
      };
      typedef std::unique_ptr<PeripheralInformation> PeripheralInformationPtr;

      ////////////////
      class MorePeripheralInformation
      {
      public:
        class Param
        {
        public:
          Param() = delete;
          Param( int aperTe, int aperT, int apar1, int apar2 )
            :perTe( aperTe )
            , perT( aperT )
            , par1( apar1 )
            , par2( apar2 )
          {}

          int perTe = 0;
          int perT = 0;
          int par1 = 0;
          int par2 = 0;
        };
      protected:
        //params
        int m_per = 0;

        //response
        std::vector<Param> m_params;
        std::map<int, Param> m_perParamsMap;

        MorePeripheralInformation( int per )
          : m_per( per )
        {}

      public:
        virtual ~MorePeripheralInformation() {}
        int getPer() const { return m_per; }
        const std::vector<Param> & getParams() const { return m_params; }

        std::map<int, Param> getPerParamsMap()
        {
          std::map<int, Param> perParamsMap;
          for ( int i = 0; i < (int)m_params.size(); i++ ) {
            if ( m_params[i].perTe != PERIPHERAL_TYPE_DUMMY ) {
              perParamsMap.insert( std::make_pair( i + m_per, m_params[i] ) );
            }
          }
          return perParamsMap;
        }
      };
      typedef std::unique_ptr<MorePeripheralInformation> MorePeripheralInformationPtr;

    } //namespace explore
  } //namespace embed
} //namespace iqrf
