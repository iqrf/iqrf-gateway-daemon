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

#include <functional>

/// iqrf namespace
namespace iqrf {

  class IWsServer {
  public:
    typedef std::function<void(const std::size_t, const std::string&)> WsServerOnMessage;

    virtual ~IWsServer() {}

    /**
     * Register message handler
     * @param handler Message handler
     */
    virtual void registerMessageHandler(WsServerOnMessage handler) = 0;

    /**
     * Unregister message handler
     */
    virtual void unregisterMessageHandler() = 0;

    /**
     * Start server listening loop
     */
    virtual void start() = 0;

    /**
     * Checks if server is listening and accepting connections
     */
    virtual bool isListening() = 0;

    /**
     * Stop listening loop, clear sessions
     */
    virtual void stop() = 0;

    /**
     * Send message to all connected clients
     * @param message Message to send
     */
    virtual void send(const std::string& message) = 0;

    /**
     * Send message to a client identified by session ID
     * @param sessionId Client session
     * @param message Message to send
     */
    virtual void send(std::size_t sessionId, const std::string& message) = 0;
  };
}
