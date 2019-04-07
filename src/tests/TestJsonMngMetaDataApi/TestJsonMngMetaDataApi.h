#pragma once

#include "IMetaDataApi.h"
#include "ITestSimulationIqrfChannel.h"
#include "ITestSimulationMessaging.h"
#include "ShapeProperties.h"
#include "ILaunchService.h"
#include "ITraceService.h"

namespace iqrf {

  class TestJsonMngMetaDataApi
  {
  public:
    TestJsonMngMetaDataApi();
    virtual ~TestJsonMngMetaDataApi();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IMetaDataApi* iface);
    void detachInterface(iqrf::IMetaDataApi* iface);

    void attachInterface(iqrf::ITestSimulationMessaging* iface);
    void detachInterface(iqrf::ITestSimulationMessaging* iface);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);
  };

}
