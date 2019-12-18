#pragma once

#include <memory>

namespace iqrf
{
  namespace embed
  {
    namespace node
    {
      ////////////////
      // Brief information about node. It has not matching DPA command
      class BriefInfo
      {
      protected:
        uint32_t m_mid = 0;
        bool m_disc = false;
        int m_hwpid = -1;
        int m_hwpidVer = -1;
        int m_osBuild = -1;
        int m_dpaVer = -1;

        BriefInfo()
        {}

      public:
        BriefInfo(unsigned mid, bool disc, int hwpid, int hwpidVer, int osBuild, int dpaVer)
          :m_mid(mid)
          , m_disc(disc)
          , m_hwpid(hwpid)
          , m_hwpidVer(hwpidVer)
          , m_osBuild(osBuild)
          , m_dpaVer(dpaVer)
        {}

        virtual ~BriefInfo() {}

        unsigned getMid() const { return m_mid; }
        bool getDisc() const { return m_disc; }
        void setDisc(bool val) { m_disc = val; }
        int getHwpid() const { return m_hwpid; }
        int getHwpidVer() const { return m_hwpidVer; }
        int getOsBuild() const { return m_osBuild; }
        int getDpaVer() const { return m_dpaVer; }

      };
      typedef std::unique_ptr<BriefInfo> BriefInfoPtr;

    } //namespace node
  } //namespace embed
} //namespace iqrf
