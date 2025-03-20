/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "IMessagingService.h"
#include "ITestSimulationMessaging.h"
#include "ShapeProperties.h"
#include "ILaunchService.h"
#include "ITraceService.h"

namespace iqrf {

  class IIqrfDpaService;

  class TestSimulationMessaging : public iqrf::IMessagingService, public iqrf::ITestSimulationMessaging
  {
  public:
    TestSimulationMessaging();
    virtual ~TestSimulationMessaging();

    //from iqrf::IMessagingService
    void registerMessageHandler(MessageHandlerFunc hndl) override;
    void unregisterMessageHandler() override;
    void sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t> & msg) override;
    bool acceptAsyncMsg() const override;
    const MessagingInstance &getMessagingInstance() const override;

    //from iqrf::ITestSimulationMessaging
    void pushIncomingMessage(const std::string& msg) override;
    std::string popOutgoingMessage(unsigned millisToWait) override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);
  private:
    class Imp;
    Imp* m_imp = nullptr;
  };

}
