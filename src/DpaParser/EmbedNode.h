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
      private:
        uint32_t m_mid;
        int m_hwpid;
        int m_hwpidVer;
        int m_osBuild;
        int m_dpaVer;
        bool m_disc;
        bool m_enm;

      public:
        BriefInfo()
          :m_mid(0)
          , m_hwpid(-1)
          , m_hwpidVer(-1)
          , m_osBuild(-1)
          , m_dpaVer(-1)
          , m_disc(false)
          , m_enm(false)
        {}

        BriefInfo(uint32_t mid)
          :m_mid(mid)
          , m_hwpid(-1)
          , m_hwpidVer(-1)
          , m_osBuild(-1)
          , m_dpaVer(-1)
          , m_disc(false)
          , m_enm(false)
        {}

        BriefInfo(uint32_t mid, int hwpid, int hwpidVer)
          :m_mid(mid)
          , m_hwpid(hwpid)
          , m_hwpidVer(hwpidVer)
          , m_osBuild(-1)
          , m_dpaVer(-1)
          , m_disc(false)
          , m_enm(false)
        {}

        BriefInfo(unsigned mid, bool disc, bool enm)
          :m_mid(mid)
          , m_hwpid(-1)
          , m_hwpidVer(-1)
          , m_osBuild(-1)
          , m_dpaVer(-1)
          , m_disc(disc)
          , m_enm(enm)
        {}

        BriefInfo(unsigned mid, bool disc, int hwpid, int hwpidVer, int osBuild, int dpaVer, bool enm)
          :m_mid(mid)
          , m_hwpid(hwpid)
          , m_hwpidVer(hwpidVer)
          , m_osBuild(osBuild)
          , m_dpaVer(dpaVer)
          , m_disc(disc)
          , m_enm(enm)
        {}

        virtual ~BriefInfo() {}

        bool isValid() const
        {
          return isMidValid() && isHwpidValid() && isHwpidVerValid() && isOsBuildValid() && isDpaVerValid();
        }

        bool getDisc() const { return m_disc; }
        void setDisc(bool val) { m_disc = val; }

        uint32_t getMid() const { return m_mid; }
        void setMid(uint32_t mid) { m_mid = mid; }
        bool isMidValid() const { return m_mid != 0; }

        int getHwpid() const { return m_hwpid; }
        void setHwpid(int hwpid) { m_hwpid = hwpid; }
        bool isHwpidValid() const { return m_hwpid >= 0; }

        int getHwpidVer() const { return m_hwpidVer; }
        void setHwpidVer(int hwpidVer) { m_hwpidVer = hwpidVer; }
        bool isHwpidVerValid() const { return m_hwpidVer >= 0; }

        int getOsBuild() const { return m_osBuild; }
        void setOsBuild(int osBuild) { m_osBuild = osBuild; }
        bool isOsBuildValid() const { return m_osBuild > 0; }

        int getDpaVer() const { return m_dpaVer; }
        void setDpaVer(int dpaVer) { m_dpaVer = dpaVer; }
        bool isDpaVerValid() const { return m_dpaVer > 0; }

        bool getEnm() const { return m_enm; }
        void setEnm(bool val) { m_enm = val; }

      };
      typedef std::unique_ptr<BriefInfo> BriefInfoPtr;

    } //namespace node
  } //namespace embed
} //namespace iqrf
