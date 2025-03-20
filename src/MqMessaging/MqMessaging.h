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
#include "TaskQueue.h"
#include "MqChannel.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include <string>

namespace iqrf {
  class MqMessaging : public IMessagingService
  {
  public:
    MqMessaging();
    virtual ~MqMessaging();

    void registerMessageHandler(MessageHandlerFunc hndl) override;
    void unregisterMessageHandler() override;
    void sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t> & msg) override;
    bool acceptAsyncMsg() const override;
    const MessagingInstance &getMessagingInstance() const override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    int handleMessageFromMq(const std::basic_string<uint8_t> & mqMessage);

    bool m_acceptAsyncMsg = false;
    MessagingInstance m_messagingInstance = MessagingInstance(MessagingType::MQ);

    MqChannel* m_mqChannel = nullptr;
    TaskQueue<std::basic_string<uint8_t>>* m_toMqMessageQueue = nullptr;

    std::string m_localMqName = "iqrf-daemon-110";
    std::string m_remoteMqName = "iqrf-daemon-100";
    uint8_t m_timeout;

    IMessagingService::MessageHandlerFunc m_messageHandlerFunc;
  };
}
