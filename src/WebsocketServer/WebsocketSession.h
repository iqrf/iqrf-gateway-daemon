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
#include <iostream>
#include <memory>
#include <string>

#include "IWebsocketSession.h"
#include "Trace.h"

#define SESSION_LOG(session, addr, port) "[" << session << "|" << addr << ":" << port << "] "
#define BEAST_ERR_LOG(ec) ec.message() << "(" << ec.category().name() << "|" << ec.value() << ")"

namespace iqrf {

  template <typename StreamType>
  class WebsocketSession : public IWebsocketSession, public std::enable_shared_from_this<WebsocketSession<StreamType>> {
  public:
    WebsocketSession() = delete;

    WebsocketSession(std::size_t id, StreamType&& stream, bool useTls)
    : m_id(id), m_stream(std::move(stream)), m_tls(useTls) {
      if constexpr (std::is_same_v<StreamType, WsStreamTls>) {
        auto endpoint = m_stream.next_layer().lowest_layer().remote_endpoint();
        m_address = endpoint.address().to_string();
        m_port = endpoint.port();
      } else {
        auto endpoint = m_stream.next_layer().socket().remote_endpoint();
        m_address = endpoint.address().to_string();
        m_port = endpoint.port();
      }
    }

    virtual ~WebsocketSession() = default;

    std::size_t getId() const override {
      return m_id;
    };

    const std::string& getAddress() const override {
      return m_address;
    };

    uint16_t getPort() const override {
      return m_port;
    };

    void send(const std::string& message) override {
      boost::asio::post(
        m_stream.get_executor(),
        [self = this->shared_from_this(), message]() mutable {
          self->m_writeQueue.push_back(std::move(message));
          if (!self->m_writing) {
            self->write();
          }
        }
      );
    }

    void run() override {
      boost::asio::dispatch(
        m_stream.get_executor(),
        boost::beast::bind_front_handler(
          &WebsocketSession::on_run,
          this->shared_from_this()
        )
      );
    }

    void close() override {
      boost::asio::post(
        m_stream.get_executor(),
        [self = this->shared_from_this()]() {
          self->m_stream.async_close(
            boost::beast::websocket::close_code::normal,
            [self](boost::beast::error_code ec) {
              if (ec) {
                TRC_WARNING(
                  SESSION_LOG(self->m_id, self->m_address, self->m_port)
                    << "Failed to close WebSocket session gracefully: "
                    << BEAST_ERR_LOG(ec)
                );
              } else {
                TRC_INFORMATION(
                  SESSION_LOG(self->m_id, self->m_address, self->m_port)
                    << "WebSocket session closed gracefully."
                );
              }
              self->shutdown_transport();
            }
          );
        }
      );
    }

    void setOnOpen(std::function<void(std::size_t)> onOpen) override {
      this->onOpen = onOpen;
    }

    void setOnClose(std::function<void(std::size_t)> onClose) override {
      this->onClose = onClose;
    }

    void setOnMessage(std::function<void(const std::size_t, const std::string&)> onMessage) override {
      this->onMessage = onMessage;
    }

  private:
    void on_run() {
      boost::beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(30));

      if constexpr (std::is_same_v<StreamType, WsStreamTls>) {
        m_stream.next_layer().async_handshake(
          boost::asio::ssl::stream_base::server,
          boost::beast::bind_front_handler(
            &WebsocketSession::on_handshake,
            this->shared_from_this()
          )
        );
      } else {
        this->accept();
      }
    }

    void on_handshake(boost::beast::error_code ec) {
      if (ec) {
        TRC_WARNING(
          SESSION_LOG(m_id, m_address, m_port)
          << "Failed to complete handshake: "
          << BEAST_ERR_LOG(ec)
        );
        this->close();
        return;
      }
      this->accept();
    }

    void accept() {
      boost::beast::get_lowest_layer(m_stream).expires_never();

      m_stream.set_option(
        boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server)
      );
      m_stream.set_option(
        boost::beast::websocket::stream_base::decorator(
          [](boost::beast::websocket::response_type& res) {
            res.set(
              boost::beast::http::field::server,
              "iqrf-gateway-daemon (" + std::string(BOOST_BEAST_VERSION_STRING) + ")"
            );
          }
        )
      );
      m_stream.async_accept(
        boost::beast::bind_front_handler(
          &WebsocketSession::on_accept,
          this->shared_from_this()
        )
      );
    }

    void on_accept(boost::beast::error_code ec) {
      if (ec) {
        TRC_WARNING(
          SESSION_LOG(m_id, m_address, m_port)
          << "Failed to accept client connection: "
          << BEAST_ERR_LOG(ec)
        );
        this->close();
      } else {
        if (this->onOpen) {
          this->onOpen(m_id);
        }
        this->read();
      }
    }

    void read() {
      m_stream.async_read(
        m_buffer,
        boost::beast::bind_front_handler(
          &WebsocketSession::on_read,
          this->shared_from_this()
        )
      );
    }

    void on_read(boost::beast::error_code ec, std::size_t bytesRead) {
      boost::ignore_unused(bytesRead);

      if (ec == boost::beast::websocket::error::closed) {
        TRC_INFORMATION(
          SESSION_LOG(m_id, m_address, m_port)
          << "Connection closed: "
          << BEAST_ERR_LOG(ec)
        );
        this->notify_server_close();
        return;
      }

      if (ec) {
        TRC_WARNING(
          SESSION_LOG(m_id, m_address, m_port)
          << "Failed to read incoming message from stream: "
          << BEAST_ERR_LOG(ec)
        );
        this->close();
        return;
      }

      if (this->onMessage) {
        std::string message = boost::beast::buffers_to_string(m_buffer.data());
        this->onMessage(m_id, message);
      }

      m_buffer.consume(m_buffer.size());
      this->read();
    }

    void write() {
      m_writing = true;
      m_stream.async_write(
        boost::asio::buffer(m_writeQueue.front()),
        boost::beast::bind_front_handler(
          &WebsocketSession::on_write,
          this->shared_from_this()
        )
      );
    }

    void on_write(boost::beast::error_code ec, std::size_t bytesWritten) {
      boost::ignore_unused(bytesWritten);
      if (ec) {
        TRC_WARNING(
          SESSION_LOG(m_id, m_address, m_port)
          << "Failed to write message to stream: "
          << BEAST_ERR_LOG(ec)
        );
        m_writeQueue.clear();
        m_writing = false;
        this->close();
      } else {
        m_writeQueue.pop_front();
        if (!m_writeQueue.empty()) {
          this->write();
        } else {
          m_writing = false;
        }
      }
    }

    void shutdown_transport() {
      if constexpr (std::is_same_v<StreamType, WsStreamTls>) {
        m_stream.next_layer().async_shutdown(
          [self = this->shared_from_this()](boost::system::error_code tls_ec) {
            boost::ignore_unused(tls_ec);
            boost::beast::error_code ec;
            self->m_stream.next_layer().lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            self->m_stream.next_layer().lowest_layer().close(ec);
            self->notify_server_close();
          }
        );
      } else {
        boost::beast::error_code ec;
        m_stream.next_layer().socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        m_stream.next_layer().socket().close(ec);
        notify_server_close();
      }
    }

    void notify_server_close() {
      if (this->onClose) {
        this->onClose(m_id);
      }
    }

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

