/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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
		const MessagingInstance& getMessagingInstance() const override;

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
