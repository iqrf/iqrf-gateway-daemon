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

#include "WebsocketSession.h"
#include "Trace.h"
#include "CryptoUtils.h"
#include "DateTimeUtils.h"
#include "WebsocketServerUtils.h"

#include <iostream>

#define SESSION_LOG(session, addr, port) "[" << session << "|" << addr << ":" << port << "] "

namespace iqrf {

  WebsocketSession::WebsocketSession(std::size_t id, boost::asio::ip::tcp::socket&& socket): m_id(id), m_stream(std::move(socket)) {
    auto endpoint = m_stream.next_layer().socket().remote_endpoint();
    m_address = endpoint.address().to_string();
    m_port = endpoint.port();
  }

  std::size_t WebsocketSession::getId() const {
    return m_id;
  }

  const std::string& WebsocketSession::getAddress() const {
    return m_address;
  }

  uint16_t WebsocketSession::getPort() const {
    return m_port;
  }

  void WebsocketSession::send(const std::string& message) {
    boost::asio::post(
      m_stream.get_executor(),
      [self = shared_from_this(), message]() mutable {
        self->m_writeQueue.push_back(std::move(message));
        if (!self->m_writing) {
          self->write();
        }
      }
    );
  }

  void WebsocketSession::run() {
    boost::asio::dispatch(
      m_stream.get_executor(),
      boost::beast::bind_front_handler(
        &WebsocketSession::on_run,
        shared_from_this()
      )
    );
  }

  void WebsocketSession::close() {
    boost::asio::post(
      m_stream.get_executor(),
      [self = shared_from_this()]() {
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
          }
        );
      }
    );
  }

  void WebsocketSession::close_auth_failed() {
    boost::asio::post(
      m_stream.get_executor(),
      [self = shared_from_this()]() {
        self->m_stream.async_close(
          boost::beast::websocket::close_code::policy_error,
          [self](boost::beast::error_code ec) {
            if (ec) {
              TRC_WARNING(
                SESSION_LOG(self->m_id, self->m_address, self->m_port)
                  << "Failed to close WebSocket session with auth failed: "
                  << BEAST_ERR_LOG(ec)
              );
            } else {
              TRC_INFORMATION(
                SESSION_LOG(self->m_id, self->m_address, self->m_port)
                  << "WebSocket session closed due to auth failure."
              );
            }
          }
        );
      }
    );
  }

  void WebsocketSession::write() {
    m_writing = true;
    m_stream.async_write(
      boost::asio::buffer(m_writeQueue.front()),
      boost::beast::bind_front_handler(
        &WebsocketSession::on_write,
        shared_from_this()
      )
    );
  }

  void WebsocketSession::on_write(boost::beast::error_code ec, std::size_t bytesWritten) {
    if (ec) {
      TRC_WARNING(
        SESSION_LOG(m_id, m_address, m_port)
        << "Failed to write message to stream: "
        << BEAST_ERR_LOG(ec)
      );
    } else {
      m_writeQueue.pop_front();
      if (!m_writeQueue.empty()) {
        this->write();
      } else {
        m_writing = false;
      }
    }
  }

  void WebsocketSession::on_run() {
    boost::beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(30));
    this->accept();
  }

  void WebsocketSession::accept() {
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
        shared_from_this()
      )
    );
  }

  void WebsocketSession::on_accept(boost::beast::error_code ec) {
    if (ec) {
      TRC_WARNING(
        SESSION_LOG(m_id, m_address, m_port)
        << "Failed to accept client connection: "
        << BEAST_ERR_LOG(ec)
      );
    } else {
      if (this->onOpen) {
        this->onOpen(m_id, ec);
      }
      this->read();
    }
  }

  void WebsocketSession::read() {
    m_stream.async_read(
      m_buffer,
      boost::beast::bind_front_handler(
        &WebsocketSession::on_read,
        shared_from_this()
      )
    );
  }

  void WebsocketSession::on_read(boost::beast::error_code ec, std::size_t bytesRead) {
    boost::ignore_unused(bytesRead);

    if (ec == boost::beast::websocket::error::closed ||
        ec == boost::asio::error::eof ||
        ec.category() == boost::asio::ssl::error::stream_category
    ) {
      TRC_INFORMATION(
        SESSION_LOG(m_id, m_address, m_port)
        << "Connection closed: "
        << BEAST_ERR_LOG(ec)
      );
      if (this->onClose) {
        this->onClose(m_id, ec);
      }
      return;
    }

    if (ec) {
      TRC_WARNING(
        SESSION_LOG(m_id, m_address, m_port)
        << "Failed to read incoming message from stream: "
        << BEAST_ERR_LOG(ec)
      );
      if (this->onClose) {
        this->onClose(m_id, ec);
      }
      return;
    }

    auto now = DateTimeUtils::get_current_timestamp();
    std::string message = boost::beast::buffers_to_string(m_buffer.data());

    bool acceptMessage = false;

    if (this->onAuth) {
      // auth possible
      if (!m_authenticated) {
        // not authenticated, check for auth
        auto ec = this->auth(message);
        if (ec) {
          // not auth message or auth failed, close
          this->send(create_error_message(ec.message()));
          this->close_auth_failed();
          if (this->onClose) {
            this->onClose(m_id, ec);
          }
          return;
        }
      } else {
        // authenticated
        if (m_expiration < now) {
          // token expired, message dropped
          auto ec = make_error_code(auth_error::expired_token);
          m_authenticated = false;
          m_expiration = -1;
          this->send(create_error_message(ec.message()));
        } else {
          // not expired, message accepted
          acceptMessage = true;
        }
      }
    } else {
      acceptMessage = true;
    }

    if (acceptMessage && this->onMessage) {
      this->onMessage(m_id, message);
    }

    //iqrfgd2;1;cPHqH+rtByPFhJCZ5nJ1uAx0FoSQfZA5ArSEpjHdPg4=

    m_buffer.consume(m_buffer.size());
    this->read();
  }

  boost::beast::error_code WebsocketSession::auth(const std::string& message) {
    nlohmann::json doc;
    try {
      doc = nlohmann::json::parse(message);
    } catch (const std::exception &e) {
      TRC_WARNING("Invalid JSON messsage received.")
      return make_error_code(auth_error::auth_failed);
    }
    auto valid = doc.is_object() && doc.size() == 1 && doc.count("auth") && doc["auth"].is_string();
    if (!valid) {
      return make_error_code(auth_error::no_auth);
    }

    auto token = doc["auth"].get<std::string>();
    uint32_t id;
    std::string key;
    try {
      CryptoUtils::parse_token(token, id, key);
    } catch (const std::exception &e) {
      TRC_WARNING(
        SESSION_LOG(m_id, m_address, m_port)
        << "Failed to parse API token: "
        << e.what()
      );
      return make_error_code(auth_error::invalid_token);
    }
    int64_t expiration = 0;
    auto ec = this->onAuth(m_id, id, key, expiration);
    if (ec) {
      TRC_WARNING(
        SESSION_LOG(m_id, m_address, m_port)
        << "Session authentication failed"
      );
    } else {
      TRC_INFORMATION(
        SESSION_LOG(m_id, m_address, m_port)
        << "Session successfully authenticated."
      );
      m_authenticated = true;
      m_expiration = expiration;
    }
    return ec;
  }

}
