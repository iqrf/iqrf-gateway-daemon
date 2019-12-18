#pragma once

#include <memory>

namespace iqrf
{
  namespace embed
  {
    namespace node
    {
      // AN passed information about node.
      class AnInfo
      {
      protected:
        uint32_t m_mid;
        int m_hwpid;
        int m_hwpidVer;

        AnInfo()
          :m_mid(0)
          , m_hwpid(-1)
          , m_hwpidVer(-1)
        {}

      public:
        AnInfo(uint32_t mid)
          :m_mid(mid)
          , m_hwpid(-1)
          , m_hwpidVer(-1)
        {}

        AnInfo(uint32_t mid, int hwpid, int hwpidVer)
          :m_mid(mid)
          , m_hwpid(hwpid)
          , m_hwpidVer(hwpidVer)
        {}

        virtual ~AnInfo() {}

        uint32_t getMid() const { return m_mid; }
        int getHwpid() const { return m_hwpid; }
        int getHwpidVer() const { return m_hwpidVer; }

      };

      ////////////////
      // Brief information about node. It has not matching DPA command
      class BriefInfo : public AnInfo
      {
      protected:
        bool m_disc = false;
        int m_osBuild = -1;
        int m_dpaVer = -1;

        BriefInfo()
        {}

      public:
        BriefInfo(unsigned mid, bool disc, int hwpid, int hwpidVer, int osBuild, int dpaVer)
          :AnInfo(mid, hwpid, hwpidVer)
          , m_disc(disc)
          , m_osBuild(osBuild)
          , m_dpaVer(dpaVer)
        {}

        virtual ~BriefInfo() {}

        bool getDisc() const { return m_disc; }
        void setDisc(bool val) { m_disc = val; }
        int getOsBuild() const { return m_osBuild; }
        int getDpaVer() const { return m_dpaVer; }

      };
      typedef std::unique_ptr<BriefInfo> BriefInfoPtr;

    } //namespace node
  } //namespace embed
} //namespace iqrf
