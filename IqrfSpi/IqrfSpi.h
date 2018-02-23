#pragma once

#include "IIqrfChannelService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {
  class ITraceService;

  class IqrfSpi : public IIqrfChannelService
  {
  public:
    class Imp;

    IqrfSpi();
    virtual ~IqrfSpi();

    State getState() const override;
    std::unique_ptr<Accessor> getAccess(ReceiveFromFunc receiveFromFunc, AccesType access) override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    Imp* m_imp = nullptr;
  };
}
