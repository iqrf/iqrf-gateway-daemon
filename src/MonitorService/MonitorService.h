/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "IMonitorService.h"
#include "ShapeProperties.h"
#include "IIqrfDpaService.h"
#include "IMessagingSplitterService.h"
#include "IUdpConnectorService.h"
#include "IWebsocketService.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {

  class MonitorService : public IMonitorService
  {
  public:
    MonitorService();

    /// \brief Destructor
    virtual ~MonitorService();

    int getDpaQueueLen() const override;
    virtual IIqrfChannelService::State getIqrfChannelState() override;
    virtual IIqrfDpaService::DpaState getDpaChannelState() override;

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
