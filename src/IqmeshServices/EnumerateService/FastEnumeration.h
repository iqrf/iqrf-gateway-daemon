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
      Enumerated(int nadr, unsigned mid, int hwpid, int hwpidVer)
        :m_nadr(nadr)
        , m_mid(mid)
        , m_hwpid(hwpid)
        , m_hwpidVer(hwpidVer)
      {}
      unsigned getMid() const const override { return m_mid; }
      int getNadr() const override { return m_nadr; }
      int getHwpid() const override { return m_hwpid; }
      int getHwpidVer() const override { return m_hwpidVer; }
    private:
      int m_nadr;
      unsigned m_mid;
      int m_hwpid;
      int m_hwpidVer;
    };
    const std::map<int, EnumeratedPtr> & getEnumerated() const override { return m_enumeratedMap; }
    const std::set<int> & getBonded() const override { return m_bonded; }
    const std::set<int> & getDiscovered() const override { return m_discovered; }
    void setBonded(const std::set<int> &bonded) { m_bonded = bonded; }
    void setDiscovered(const std::set<int> &discovered) { m_discovered = discovered; }
    void addItem(int nadr, unsigned mid, int hwpid, int hwpidVer)
    {
      m_enumeratedMap.insert(std::make_pair(nadr, EnumeratedPtr(shape_new Enumerated(nadr, mid, hwpid, hwpidVer))));
    }
  private:
    std::map<int, EnumeratedPtr> m_enumeratedMap;
    std::set<int> m_bonded;
    std::set<int> m_discovered;
  };
}
