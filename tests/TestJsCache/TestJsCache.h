#pragma once

#include "ShapeProperties.h"
#include "IJsRenderService.h"
#include "IJsCacheService.h"
#include "ITestSimulationIRestApiService.h"
#include "ILaunchService.h"
#include "IConfigurationService.h"
#include "ITraceService.h"

namespace iqrf {

  class TestJsCache
  {
  public:
    TestJsCache();
    virtual ~TestJsCache();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IJsRenderService* iface);
    void detachInterface(iqrf::IJsRenderService* iface);

    void attachInterface(iqrf::IJsCacheService* iface);
    void detachInterface(iqrf::IJsCacheService* iface);

    void attachInterface(shape::ITestSimulationIRestApiService* iface);
    void detachInterface(shape::ITestSimulationIRestApiService* iface);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(shape::IConfigurationService* iface);
    void detachInterface(shape::IConfigurationService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);
  };

}
