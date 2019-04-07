#pragma once

#include "ILaunchService.h"
#include "ISchedulerService.h"
#include "IUdpConnectorService.h"
#include "IMessagingSplitterService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <map>

namespace iqrf {
  class JsonMngApi
  {
  public:
    JsonMngApi();
    virtual ~JsonMngApi();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(ISchedulerService* iface);
    void detachInterface(ISchedulerService* iface);

    void attachInterface(IUdpConnectorService* iface);
    void detachInterface(IUdpConnectorService* iface);

    void attachInterface(IMessagingSplitterService* iface);
    void detachInterface(IMessagingSplitterService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
