#pragma once

#include "ShapeDefines.h"
#include "stdint.h"
#include <string>
#include <set>

namespace iqrf {
  /// \class IEnumerateService
  class IEnumerateService
  {
  public:
    class CoordinatorData
    {
      //TODO getters, setters
    public:
      std::set<int> m_bonded;
      std::set<int> m_discovered;
      bool m_valid = false;
      //TODO VRN, Zone, Parent
    };

    class NodeData
    {
      //TODO getters, setters
    public:
      NodeData()
      {}

      NodeData(unsigned mid, int nadr, int hwpid, int hwpidVer)
        : m_mid(mid)
        , m_nadr(nadr)
        , m_hwpid(hwpid)
        , m_hwpidVer(hwpidVer)
      {}

      unsigned getMid() const { return m_mid; }
      int getNadr() const { return m_nadr; }
      int getHwpid() const { return m_hwpid; }
      int getHwpidVer() const { return m_hwpidVer; }
      int getOsBuild() const { return m_osBuild; }
      int getOsVer() const { return m_osVer; }
      int getDpaVer() const { return m_dpaVer; }
      bool getModeStd() const { return m_modeStd; }
      bool getStdAndLpNet() const { return m_stdAndLpNet; }
      std::set<int> getEmbedPer() { return m_embedPer; }
      std::set<int> getUserPer() { return m_userPer; }
      
      bool isValid() const { return m_valid; }

      void setMid(unsigned v) { m_mid = v; }
      void setNadr(int v) { m_nadr = v; }
      void setHwpid(int v) { m_hwpid = v; }
      void setHwpidVer(int v) { m_hwpidVer = v; }
      void setOsBuild(int v) { m_osBuild = v; }
      void setOsVer(int v) { m_osVer = v; }
      void setDpaVer(int v) { m_dpaVer = v; }
      void setModeStd(bool v) { m_modeStd = v; }
      void setStdAndLpNet(bool v) { m_stdAndLpNet = v; }
      void setEmbedPer(const std::set<int> & v) { m_embedPer = v; }
      void setUserPer(const std::set<int> & v) { m_userPer = v; }

      void setValid(bool v) { m_valid = v; }

    private:
      unsigned m_mid = 0;
      int m_nadr = -1;
      int m_hwpid = 0;
      int m_hwpidVer = 0;
      int m_osBuild = 0;
      int m_osVer = 0;
      int m_dpaVer = 0;
      bool m_modeStd = true;
      bool m_stdAndLpNet = false;
      bool m_valid = false;

      std::set<int> m_embedPer;
      std::set<int> m_userPer;
      
      //TODO others
    };

    CoordinatorData virtual getCoordinatorData() const = 0;
    NodeData virtual getNodeData(uint16_t nadr) const = 0;
    
    virtual ~IEnumerateService() {}
  };
}
