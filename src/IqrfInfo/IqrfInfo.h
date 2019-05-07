#pragma once

#include "ShapeProperties.h"
#include "ITraceService.h"
#include "IIqrfDpaService.h"
#include "IEnumerateDeviceService.h"
#include "ILaunchService.h"

namespace iqrf {
  //TODO
  class IIqrfInfo
  {
  public:
    virtual ~IIqrfInfo() {}
  };

  class IqrfInfo : public IIqrfInfo
  {
  public:
    IqrfInfo();
    virtual ~IqrfInfo();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IEnumerateDeviceService* iface);
    void detachInterface(iqrf::IEnumerateDeviceService* iface);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
