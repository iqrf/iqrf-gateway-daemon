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

#include "IMessagingService.h"
#include "TaskQueue.h"
#include "MqChannel.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {
  class MqMessaging : public IMessagingService
  {
  public:
    MqMessaging();
    virtual ~MqMessaging();

    void registerMessageHandler(MessageHandlerFunc hndl) override;
    void unregisterMessageHandler() override;
    void sendMessage(const std::string& messagingId, const std::basic_string<uint8_t> & msg) override;
    const std::string & getName() const override { return m_name; }
    bool acceptAsyncMsg() const override { return m_acceptAsyncMsg; }

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    int handleMessageFromMq(const std::basic_string<uint8_t> & mqMessage);

    std::string m_name;
    bool m_acceptAsyncMsg = false;

    MqChannel* m_mqChannel = nullptr;
    TaskQueue<std::basic_string<uint8_t>>* m_toMqMessageQueue = nullptr;

    std::string m_localMqName = "iqrf-daemon-110";
    std::string m_remoteMqName = "iqrf-daemon-100";

    IMessagingService::MessageHandlerFunc m_messageHandlerFunc;
  };
}
