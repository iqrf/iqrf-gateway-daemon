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
        unsigned m_mid;
        int m_hwpid;
        int m_hwpidVer;
        int m_osBuild;
        int m_dpaVer;

        BriefInfo()
        {}

        BriefInfo(unsigned mid, int hwpid, int hwpidVer, int osBuild, int dpaVer)
          :m_mid(mid)
          , m_hwpid(hwpid)
          , m_hwpidVer(hwpidVer)
          , m_osBuild(osBuild)
          , m_dpaVer(dpaVer)
        {}

      public:
        virtual ~BriefInfo() {}

        unsigned getMid() const { return m_mid; }
        int getHwpid() const { return m_hwpid; }
        int getHwpidVer() const { return m_hwpidVer; }
        int getOsBuild() const { return m_osBuild; }
        int getDpaVer() const { return m_dpaVer; }

      };
      typedef std::unique_ptr<BriefInfo> BriefInfoPtr;

    } //namespace node
  } //namespace embed
} //namespace iqrf
