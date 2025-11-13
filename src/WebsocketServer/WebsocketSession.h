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

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>

#include <cstdint>
#include <deque>
#include <memory>
#include <string>

#include "IWebsocketSession.h"

#define BEAST_ERR_LOG(ec) ec.message() << "(" << ec.category().name() << "|" << ec.value() << ")"

namespace iqrf {

  template <typename StreamType>
  class WebsocketSession : public IWebsocketSession, public std::enable_shared_from_this<WebsocketSession<StreamType>> {
  public:
    WebsocketSession() = delete;

    WebsocketSession(std::size_t id, StreamType&& stream, bool useTls);

    virtual ~WebsocketSession() = default;

    std::size_t getId() const override;

    const std::string& getAddress() const override;

    uint16_t getPort() const override;

    void send(const std::string& message) override;

    void run() override;

    void close() override;

    void setOnOpen(std::function<void(std::size_t)> onOpen) override;

    void setOnClose(std::function<void(std::size_t)> onClose) override;

    void setOnMessage(std::function<void(const std::size_t, const std::string&)> onMessage) override;

  private:
    void on_run();

    void on_handshake(boost::beast::error_code ec);

    void accept();

    void on_accept(boost::beast::error_code ec);

    void read();

    void on_read(boost::beast::error_code ec, std::size_t bytesRead);

    void write();

    void on_write(boost::beast::error_code ec, std::size_t bytesWritten);

    void shutdown_transport();

    void notify_server_close();

    /// Session ID
    std::size_t m_id;
    /// IP address
    std::string m_address;
    /// Port number
    uint16_t m_port;
    /// Socket/stream
    StreamType m_stream;
    /// TLS connection
    bool m_tls;
    /// Receive buffer
    boost::beast::flat_buffer m_buffer;
    /// Writing queue
    std::deque<std::string> m_writeQueue;
    /// Writing in progress
    bool m_writing;
    /// On open callback
    std::function<void(std::size_t)> onOpen;
    /// On close callback
    std::function<void(std::size_t)> onClose;
    /// On message callback
    std::function<void(const std::size_t, const std::string&)> onMessage;
  };

}

