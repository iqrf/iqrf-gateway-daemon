#pragma once

#include "IEnumerateService.h"
#include <map>

namespace iqrf
{
  class FastEnumeration : public IEnumerateService::IFastEnumeration
  {
  public:
    class Item : public IEnumerateService::IFastEnumeration::Item
    {
    public:
      Item() = delete;
      Item(int nadr, unsigned mid, int hwpid, int hwpidVer)
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
    const std::map<int, ItemPtr> & getItems() const override { return m_items; }
    void addItem(int nadr, unsigned mid, int hwpid, int hwpidVer)
    {
      m_items.insert(std::make_pair(nadr, ItemPtr(shape_new Item(nadr, mid, hwpid, hwpidVer))));
    }
  private:
    std::map<int, ItemPtr> m_items;
  };
}
