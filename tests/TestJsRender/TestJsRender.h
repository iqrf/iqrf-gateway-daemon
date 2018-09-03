#pragma once

#include "IJsRenderService.h"
#include "ShapeProperties.h"
#include "ILaunchService.h"
#include "ITraceService.h"

namespace iqrf {

  class TestJsRender
  {
  public:
    TestJsRender();
    virtual ~TestJsRender();
  
    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IJsRenderService* iface);
    void detachInterface(iqrf::IJsRenderService* iface);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);
  };

}
