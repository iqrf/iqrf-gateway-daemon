#pragma once

#include "JsDriver.h"

namespace iqrf
{
  namespace embed
  {
    namespace explore
    {
      ////////////////
      class Enumerate : public JsDpaRequest
      {
      private:
        int m_dpaVer = 0;
        int m_perNr = 0;;
        std::vector<int> m_embeddedPers;
        int m_hwpid = 0;
        int m_hwpidVer = 0;
        int m_flags = 0;
        std::vector<int> m_userPer;

      public:
        Enumerate(uint16_t nadr, uint16_t hwpid, IJsRenderService* iJsRenderService)
          :JsDpaRequest(nadr, hwpid, iJsRenderService)
        {
        }

        std::string getFunctionName() const override
        {
          return "iqrf.embed.explore.Enumerate";
        }

        void parseResponse(const rapidjson::Value& v) override
        {
          using namespace rapidjson;
          try {
            //TODO use rapidjson::pointers ?
            m_dpaVer = jutils::getMemberAs<int>("dpaVer", v);
            m_perNr = jutils::getMemberAs<int>("perNr", v);
            m_embeddedPers = jutils::getMemberAsVector<int>("embeddedPers", v);
            m_hwpid = jutils::getMemberAs<int>("hwpid", v);
            m_hwpidVer = jutils::getMemberAs<int>("hwpidVer", v);
            m_flags = jutils::getMemberAs<int>("flags", v);
            m_userPer = jutils::getMemberAsVector<int>("userPer", v);
            m_valid = true;
          }
          catch (std::exception & e) {
            TRC_WARNING("invalid data: " << e.what());
            m_valid = false;
          }
        }

        // get data as returned from driver
        int getDpaVer() const { return m_dpaVer; }
        int getPerNr() const { return m_perNr; }
        std::vector<int> getEmbeddedPers() const { return m_embeddedPers; }
        int getHwpid() const { return m_hwpid; }
        int getHwpidVer() const { return m_hwpidVer; }
        int getFlags() const { return m_flags; }
        std::vector<int> getUserPer() const { return m_userPer; }

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

    } //namespace explore
  } //namespace embed
} //namespace iqrf
