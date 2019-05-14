#pragma once

#include "ShapeDefines.h"
#include "stdint.h"
#include <string>
#include <list>
#include <vector>

namespace iqrf {
  /// \class IEnumerateService
  class IEnumerateService
  {
  public:
    class CoordinatorData
    {
      //TODO getters, setters
    public:
      std::vector<int> m_bonded;
      std::vector<int> m_discovered;
      bool m_valid = false;
      //TODO VRN, Zone, Parent
    };

    class NodeData
    {
      //TODO getters, setters
    public:
      int m_hwpid = 0;
      int m_hwpidVer = 0;
      int m_osBuild = 0;
      int m_osVer = 0;
      unsigned m_mid = 0;
      int m_dpaVersion = 0;
      bool m_modeStd = true;
      bool m_stdAndLpNetwork = false;
      bool m_valid = false;
      //TODO others
    };

    CoordinatorData virtual getCoordinatorData() const = 0;
    NodeData virtual getNodeData(uint16_t nadr) const = 0;
    
    virtual ~IEnumerateService() {}
  };
}
