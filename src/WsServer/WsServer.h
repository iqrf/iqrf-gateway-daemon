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
#include "WsSession.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>

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
    /**
     * Listen for incoming client connections
     */
    void accept();

    /**
     * Client connection onAccept callback
     * @param ec Error code
     * @param socket TCP socket
     */
    void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);

    /**
     * Register new client session
     * @param session Client session
     */
    void registerSession(std::shared_ptr<WsSession> session);

    /**
     * Unregister client session
     * @param sessionId Session ID
     */
    void unregisterSession(const std::size_t sessionId);

    /// Launcher service
    shape::ILaunchService *m_launchService = nullptr;
    /// Path to directory with certificates
    std::string m_certDir;
    /// Server address
    boost::asio::ip::address m_address;
    /// Server port
    uint16_t m_port;
    /// Path to certificate file
    std::optional<std::string> m_certPath;
    /// Path to private key file
    std::optional<std::string> m_keyPath;
    /// Accept only localhost connections
    bool m_acceptOnlyLocalhost;
    /// Thread
    std::thread m_thread;
    /// IO context
    std::optional<boost::asio::io_context> m_ioc = std::nullopt;
    /// SSL context
    std::optional<boost::asio::ssl::context> m_ctx = std::nullopt;
    /// TCP acceptor
    std::optional<boost::asio::ip::tcp::acceptor> m_acceptor;
    /// IO context work guard
    std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> m_workGuard;
    /// Session ID
    std::size_t m_sessionCounter = 0;
    /// Session registry mutex
    std::mutex m_mutex;
    /// Session registry
    std::unordered_map<size_t, std::shared_ptr<WsSession>> m_sessionRegistry;
    /// On message handler
    WsServerOnMessage m_onMessage;
  };
}
