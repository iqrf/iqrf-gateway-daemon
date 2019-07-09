#pragma once

#include "IEnumerateService.h"
#include <map>

namespace iqrf
{
  class FastEnumeration : public IEnumerateService::IFastEnumeration
  {
  public:
    class Enumerated : public IEnumerateService::IFastEnumeration::Enumerated
    {
    public:
      Enumerated() = delete;
      Enumerated(int nadr, unsigned mid, int hwpid, int hwpidVer, int osBuild, int osVer, int dpaVer, IEnumerateService::INodeDataPtr nodeDataPtr)
        :m_nadr(nadr)
        , m_mid(mid)
        , m_hwpid(hwpid)
        , m_hwpidVer(hwpidVer)
        , m_osBuild(osBuild)
        , m_osVer(osVer)
        , m_dpaVer(dpaVer)
        , m_nodeDataPtr(std::move(nodeDataPtr))
      {}
      unsigned getMid() const override { return m_mid; }
      int getNadr() const override { return m_nadr; }
      int getHwpid() const override { return m_hwpid; }
      int getHwpidVer() const override { return m_hwpidVer; }
      int getOsBuild() const override { return m_osBuild; }
      int getOsVer() const override { return m_osVer; }
      int getDpaVer() const override { return m_dpaVer; }
      IEnumerateService::INodeDataPtr getNodeData() override { return std::move(m_nodeDataPtr); }
      virtual ~Enumerated() {}

    private:
      int m_nadr;
      unsigned m_mid;
      int m_hwpid;
      int m_hwpidVer;
      int m_osBuild;
      int m_osVer;
      int m_dpaVer;
      IEnumerateService::INodeDataPtr m_nodeDataPtr;
    };
    const std::map<int, EnumeratedPtr> & getEnumerated() const override { return m_enumeratedMap; }
    const std::set<int> & getBonded() const override { return m_bonded; }
    const std::set<int> & getDiscovered() const override { return m_discovered; }
    const std::set<int> & getNonDiscovered() const override { return m_nonDiscovered; }
    void setBondedDiscovered(const std::set<int> &bonded, const std::set<int> &discovered)
    {
      m_bonded = bonded;
      m_discovered = discovered;
      for (auto i : m_bonded) {
        if (m_discovered.find(i) == m_discovered.end()) {
          m_nonDiscovered.insert(i);
        }
      }
    }
    void addItem(int nadr, unsigned mid, int hwpid, int hwpidVer, int osBuild, int osVer, int dpaVer, IEnumerateService::INodeDataPtr nodeDataPtr)
    {
      m_enumeratedMap.insert(std::make_pair(nadr, EnumeratedPtr(shape_new Enumerated(nadr, mid, hwpid, hwpidVer, osBuild, osVer, dpaVer, std::move(nodeDataPtr)))));
    }
    virtual ~FastEnumeration() {}
  private:
    std::map<int, EnumeratedPtr> m_enumeratedMap;
    std::set<int> m_bonded;
    std::set<int> m_discovered;
    std::set<int> m_nonDiscovered;
  };
}
