#pragma once

#include "CdcInterface.h"
#include "CDCImpl.h"
#include "IIqrfChannelService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <string>

namespace iqrfgw {
  class ITraceService;

  class IqrfCdc : public IIqrfChannelService
  {
  public:
    IqrfCdc();
    virtual ~IqrfCdc();

    void sendTo(const std::basic_string<unsigned char>& message) override;
    void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc) override;
    void unregisterReceiveFromHandler() override;
    State getState() const override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    CDCImpl* m_cdc = nullptr;
    ReceiveFromFunc m_receiveFromFunc;
    std::string m_interfaceName;
  };
}
