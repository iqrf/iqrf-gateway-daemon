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

#include "IWsServer.h"
#include "ILaunchService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"

namespace iqrf {

  class WsServer : public IWsServer {
  public:
    /**
     * Constructor
     */
    WsServer();

    /**
     * Destructor
     */
    virtual ~WsServer();

    /**
     * Register message handler
     * @param handler Message handler
     */
    void registerMessageHandler(WsServerOnMessage handler) override;

    /**
     * Unregister message handler
     */
    void unregisterMessageHandler() override;

    /**
     * Start server listening loop
     */
    void start() override;

    /**
     * Checks if server is listening and accepting connections
     */
    bool isListening() override;

    /**
     * Stop listening loop, clear sessions
     */
    void stop() override;

    /**
     * Send message to all connected clients
     * @param message Message to send
     */
    void send(const std::string& message) override;

    /**
     * Send message to a client identified by session ID
     * @param sessionId Client session
     * @param message Message to send
     */
    void send(const std::size_t sessionId, const std::string& message) override;

    /**
     * Initializes component
     * @param props Component configuration
     */
    void activate(const shape::Properties *props = 0);

    /**
     * Modifies component properties
     * @param props Component configuration
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
     * Attaches tracing service interface
     * @param iface Tracing service interface
     */
    void attachInterface(shape::ITraceService *iface);

    /**
     * Detaches tracing service interface
     * @param iface Tracing service interface
     */
    void detachInterface(shape::ITraceService *iface);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
}
