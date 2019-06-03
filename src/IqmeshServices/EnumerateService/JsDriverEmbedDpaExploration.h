#pragma once

#include "JsDriverRequest.h"
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
      class Enumerate : public JsDriverRequest
      {
      private:
        int m_dpaVer = 0;
        int m_perNr = 0;;
        std::set<int> m_embedPer;
        int m_hwpid = 0;
        int m_hwpidVer = 0;
        int m_flags = 0;
        std::set<int> m_userPer;

      public:
        Enumerate(uint16_t nadr)
          :JsDriverRequest(nadr)
        {
        }

        std::string functionName() const override
        {
          return "iqrf.embed.explore.Enumerate";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          //TODO use rapidjson::pointers
          m_dpaVer = jutils::getMemberAs<int>("dpaVer", v);
          m_perNr = jutils::getMemberAs<int>("perNr", v);
          {
            auto vect = jutils::getMemberAsVector<int>("embeddedPers", v);
            m_embedPer = std::set<int>(vect.begin(), vect.end());
          }
          m_hwpid = jutils::getMemberAs<int>("hwpid", v);
          m_hwpidVer = jutils::getMemberAs<int>("hwpidVer", v);
          m_flags = jutils::getMemberAs<int>("flags", v);
          {
            auto vect = jutils::getMemberAsVector<int>("userPer", v);
            m_userPer = std::set<int>(vect.begin(), vect.end());
          }
        }

        // get data as returned from driver
        int getDpaVer() const { return m_dpaVer; }
        int getPerNr() const { return m_perNr; }
        std::set<int> getEmbedPer() const { return m_embedPer; }
        int getHwpid() const { return m_hwpid; }
        int getHwpidVer() const { return m_hwpidVer; }
        int getFlags() const { return m_flags; }
        std::set<int> getUserPer() const { return m_userPer; }

        // get more detailed data parsing
        std::string getDpaVerAsString() const
        {
          std::ostringstream os;
          os.fill('0');
          os << std::hex <<
            std::setw(2) << ((m_dpaVer & 0xefff) >> 8) << '.' << std::setw(2) << (m_dpaVer & 0xff);
          return os.str();
        }

        bool isModeStd() const { return (m_flags & 1) != 0; }
        bool isModeLp() const { return (m_flags & 1) == 0; }
        bool isStdAndLpSupport() const { return (m_flags & 0b100) != 0; }

      };

      ////////////////
      class PeripheralInformation : public JsDriverRequest, public IEnumerateService::IPeripheralInformationData
      {
      private:
        //params
        int m_per = 0;

        //response
        int m_perTe = 0;
        int m_perT = 0;
        int m_par1 = 0;
        int m_par2 = 0;

      public:
        PeripheralInformation(uint16_t nadr, int per)
          :JsDriverRequest(nadr)
          , m_per(per)
        {
        }

        std::string functionName() const override
        {
          return "iqrf.embed.explore.PeripheralInformation";
        }

        std::string requestParameter() const override
        {
          using namespace rapidjson;
          Document par;

          Pointer("/per").Set(par, (int)m_per);

          std::string parStr;
          StringBuffer buffer;
          Writer<rapidjson::StringBuffer> writer(buffer);
          par.Accept(writer);
          parStr = buffer.GetString();

          return parStr;
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;

          m_perTe = jutils::getMemberAs<int>("perTe", v);
          m_perT = jutils::getMemberAs<int>("perT", v);
          m_par1 = jutils::getMemberAs<int>("par1", v);
          m_par2 = jutils::getMemberAs<int>("par2", v);
        }

        // get data as returned from driver
        int getPerTe() const override { return m_perTe; }
        int getPerT() const override { return m_perT; }
        int getPar1() const override { return m_par1; }
        int getPar2() const override { return m_par2; }

        // get more detailed data parsing

      };
    } //namespace explore
  } //namespace embed
} //namespace iqrf
