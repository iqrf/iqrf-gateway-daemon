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

#include "IApiTokenService.h"
#include "IMessagingService.h"
#include "ILaunchService.h"
#include "ITraceService.h"
#include "IModeService.h"
#include "ShapeProperties.h"

/// iqrf namespace
namespace iqrf {
  class WebsocketMessaging : public IMessagingService {
  public:
    /**
     * Constructor
     */
    WebsocketMessaging();

    /**
     * Destructor
     */
    virtual ~WebsocketMessaging();

    /**
     * Register message handler
     * @param handler Message handler
     */
    void registerMessageHandler(MessageHandlerFunc handler) override;

    /***
     * Unregister message handler
     */
    void unregisterMessageHandler() override;

    /**
     * Send message via websocket
     * @param messaging Messaging instance
     * @param msg Message to send
     */
    void sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t>& msg) override;

    /**
     * Returns asynchronous message accepting policy
     */
    bool acceptAsyncMsg() const override;

    /**
     * Returns messaging instance
     */
    const MessagingInstance& getMessagingInstance() const override;

    /**
     * Initializes component
     * @param props Component properties
     */
    void activate(const shape::Properties *props = 0);

    /**
     * Modifies component properties
     * @param props Component properties
     */
    void modify(const shape::Properties *props);

    /**
     * Deactivates component
     */
    void deactivate();

    /**
     * Attaches launcher service interface
     * @param iface Launcher service interface
     */
    void attachInterface(shape::ILaunchService *iface);

    /**
     * Detaches launcher service interface
     * @param iface Launcher service interface
     */
    void detachInterface(shape::ILaunchService *iface);

    /**
     * Attaches API token service interface
     * @param iface API token service interface
     */
    void attachInterface(IApiTokenService *iface);

    /**
     * Detaches API token service interface
     * @param iface API token service interface
     */
    void detachInterface(IApiTokenService *iface);

    /**
     * Attaches mode service interface
     * @param iface Mode service interface
     */
    void attachInterface(IModeService *iface);

    /**
     * Detaches mode service interface
     * @param iface Mode service interface
     */
    void detachInterface(IModeService *iface);

    /**
     * Attaches tracing service interface
     * @param iface Tracing service interface
     */
    void attachInterface(shape::ITraceService* iface);

    /**
     * Detaches tracing service interface
     * @param iface Tracing service interface
     */
    void detachInterface(shape::ITraceService* iface);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
}
