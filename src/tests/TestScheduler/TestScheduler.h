#pragma once

#include "ISchedulerService.h"
#include "ShapeProperties.h"
#include "ILaunchService.h"
#include "ITraceService.h"

namespace iqrf {

  class TestScheduler
  {
  public:
    TestScheduler();
    virtual ~TestScheduler();
  
    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::ISchedulerService* iface);
    void detachInterface(iqrf::ISchedulerService* iface);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);
  };

}
