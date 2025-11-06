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

#include "WebsocketCallbackTypes.h"
#include "WebsocketServerParams.h"
#include "ILaunchService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"

namespace iqrf {

  class WebsocketServer {
  public:
    /// on message callback

    /**
     * Default constructor
     */
    explicit WebsocketServer(const WebsocketServerParams& params);

    /**
     * Constructor with onMessage callback
     */
    WebsocketServer(const WebsocketServerParams& params, WsServerOnMessage onMessage, WsServerOnAuth onAuth);

    /**
     * Destructor
     */
    virtual ~WebsocketServer();

    /**
     * Start server listening loop
     */
    void start();

    /**
     * Checks if server is listening and accepting connections
     */
    bool isListening();

    /**
     * Stop listening loop, clear sessions
     */
    void stop();

    /**
     * Send message to all connected clients
     * @param message Message to send
     */
    void send(const std::string& message);

    /**
     * Send message to a client identified by session ID
     * @param sessionId Client session
     * @param message Message to send
     */
    void send(const std::size_t sessionId, const std::string& message);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
}
