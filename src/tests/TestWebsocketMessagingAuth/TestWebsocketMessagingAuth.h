#pragma once

#include "IApiTokenService.h"
#include "ITestSimulationIqrfChannel.h"
#include "IMessagingSplitterService.h"
#include "ShapeProperties.h"
#include "ILaunchService.h"
#include "ITraceService.h"

namespace iqrf {

  class TestWebsocketMessagingAuth
  {
  public:
    TestWebsocketMessagingAuth();
    virtual ~TestWebsocketMessagingAuth();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::ITestSimulationIqrfChannel* iface);
    void detachInterface(iqrf::ITestSimulationIqrfChannel* iface);

    void attachInterface(iqrf::IApiTokenService* iface);
    void detachInterface(iqrf::IApiTokenService* iface);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);
  };

}
