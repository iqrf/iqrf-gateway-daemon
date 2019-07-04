#pragma once

#include "IGwMonitorService.h"
#include "ShapeProperties.h"
#include "IIqrfDpaService.h"
#include "IMessagingSplitterService.h"
#include "IUdpConnectorService.h"
#include "IWebsocketService.h"
#include "ITraceService.h"
#include <string>


namespace iqrf {

  class GwMonitorService : public IGwMonitorService
  {
  public:
    GwMonitorService();

    /// \brief Destructor
    virtual ~GwMonitorService();

    int getDpaQueueLen() const override;
    virtual IIqrfChannelService::State getIqrfChannelState() override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(iqrf::IMessagingSplitterService* iface);
    void detachInterface(iqrf::IMessagingSplitterService* iface);

    void attachInterface(iqrf::IUdpConnectorService* iface);
    void detachInterface(iqrf::IUdpConnectorService* iface);

    void attachInterface(shape::IWebsocketService* iface);
    void detachInterface(shape::IWebsocketService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
