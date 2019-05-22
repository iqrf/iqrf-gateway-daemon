#pragma once

#include "JsDriverRequest.h"
#include "JsonUtils.h"
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
      class Read : public JsDriverRequest
      {
      private:
        unsigned m_mid = 0;
        int m_osVersion = 0;;
        int m_trMcuType = 0;
        int m_osBuild = 0;
        int m_rssi = 0;
        double m_supplyVoltage = 0;
        int m_flags = 0;
        int m_slotLimits = 0;
        std::vector<int> m_ibk;

      public:
        Read(uint16_t nadr)
          :JsDriverRequest(nadr)
        {
        }

        std::string functionName() const override
        {
          return "iqrf.embed.os.Read";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;
          //TODO use rapidjson::pointers ?
          //{
          //  const Value *val = Pointer("/mid").Get(v);
          //  if (val && val->IsInt()) {
          //    m_mid = val->GetInt();
          //  }
          //}
          m_mid = jutils::getMemberAs<unsigned>("mid", v);
          m_osVersion = jutils::getMemberAs<int>("osVersion", v);
          m_trMcuType = jutils::getMemberAs<int>("trMcuType", v);
          m_osBuild = jutils::getMemberAs<int>("osBuild", v);
          m_rssi = jutils::getMemberAs<int>("rssi", v);
          m_supplyVoltage = jutils::getMemberAs<double>("supplyVoltage", v);
          m_flags = jutils::getMemberAs<int>("flags", v);
          m_ibk = jutils::getPossibleMemberAsVector<int>("ibk", v, m_ibk);
        }

        // get data as returned from driver
        unsigned getMid() const { return m_mid; }
        int getOsVersion() const { return m_osVersion; }
        int getTrMcuType() const { return m_trMcuType; }
        int getOsBuild() const { return m_osBuild; }
        int getRssi() const { return m_rssi; }
        double getSupplyVoltage() const { return m_supplyVoltage; }
        int getFlags() const { return m_flags; }
        int getSlotLimits() const { return m_slotLimits; }
        const std::vector<int> & getIbk() const { return m_ibk; }

        // get more detailed data parsing
        std::string getMidAsString() const
        {
          std::ostringstream moduleId;
          moduleId.fill('0');
          moduleId << std::hex << std::uppercase <<
            std::setw(2) << (int)(0xFF & (m_mid >> 24)) <<
            std::setw(2) << (int)(0xFF & (m_mid >> 16)) <<
            std::setw(2) << (int)(0xFF & (m_mid >> 8)) <<
            std::setw(2) << (int)(0xFF & (m_mid));
          return moduleId.str();
        }

        std::string getOsVersionAsString() const
        {
          std::ostringstream os;
          os << std::hex << (int)(m_osVersion >> 4) << '.';
          os.fill('0');
          os << std::setw(2) << (int)(m_osVersion & 0xf) << 'D';
          return os.str();
        }

        std::string getTrTypeAsString() const
        {
          std::string trTypeStr = "(DC)TR-";
          switch (m_trMcuType >> 4) {
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

        bool isFccCertified() const { return m_trMcuType & 0x08 != 0; }

        std::string getTrMcuTypeAsString() const { return ((m_trMcuType & 0x07) == 0x04) ? "PIC16LF1938" : "UNKNOWN"; }

        std::string getOsBuildAsString() const
        {
          std::ostringstream os;
          os.fill('0'); os.width(4);
          os << std::hex << std::uppercase << m_osBuild;
          return os.str();
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
          os << std::setprecision(2) << getSupplyVoltage() << " V";
          return os.str();
        }

        bool isInsufficientOsBuild() const { return m_flags & 0x01 != 0; }
        bool getInterface() const { return m_flags & 0x02 != 0; }
        std::string getInterfaceAsString() const { return (m_flags & 0x02) != 0 ? "UART" : "SPI"; }
        bool isDpaHandlerDetected() const { return m_flags & 0x04 != 0; }
        bool isDpaHandlerNotDetectedButEnabled() const { return m_flags & 0x08 != 0; }
        bool isNoInterfaceSupported() const { return m_flags & 0x10 != 0; }

        int getShortestTimeSlot() const
        {
          int s = ((m_slotLimits & 0x0f) + 3) * 10;
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
          int s = (((m_slotLimits >> 0x04) & 0x0f) + 3) * 10;
          return  s;
        }

        std::string getLongestTimeSlotAsString() const
        {
          std::ostringstream os;
          os << getLongestTimeSlot() << " ms";
          return os.str();
        }

      };

      ////////////////
      class ReadCfg : public JsDriverRequest
      {
      private:
        unsigned m_checkum = 0;
        std::vector<int> m_configuration;
        int m_rfpgm = 0;
        int m_undocumented = 0;

      public:
        ReadCfg(uint16_t nadr)
          :JsDriverRequest(nadr)
        {
        }

        std::string functionName() const override
        {
          return "iqrf.embed.os.ReadCfg";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          m_checkum = jutils::getMemberAs<int>("checksum", v);
          m_configuration = jutils::getMemberAsVector<int>("configuration", v);
          m_rfpgm = jutils::getMemberAs<int>("rfpgm", v);
          m_undocumented = jutils::getMemberAs<int>("undocumented", v);
        }

        // get data as returned from driver
        unsigned getCheckum() const { return m_checkum; }
        std::vector<int> getConfiguration() const { return m_configuration; }
        int getRfpgm() const { return m_rfpgm; }
        int getUndocumented() const { return m_undocumented; }

        // get more detailed data parsing
        // TODO

      };

    } //namespace os
  } //namespace embed
} //namespace iqrf