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

#include "IWebsocketSession.h"

#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <string>

#include "IWebsocketSession.h"
#include "CryptoUtils.h"
#include "DateTimeUtils.h"
#include "WebsocketServerUtils.h"
#include "Trace.h"

#define SESSION_LOG(session, addr, port) "[" << session << "|" << addr << ":" << port << "] "
#define BEAST_ERR_LOG(ec) ec.message() << "(" << ec.category().name() << "|" << ec.value() << ")"

namespace iqrf {

  template <typename StreamType>
  class WebsocketSession : public IWebsocketSession, public std::enable_shared_from_this<WebsocketSession<StreamType>> {
  private:
    /// Session ID
    const std::size_t m_id;
    /// IP address
    std::string m_address;
    /// Port number
    uint16_t m_port;
    /// Socket/stream
    StreamType m_stream;
    /// Receive buffer
    boost::beast::flat_buffer m_buffer;
    /// Writing queue
    std::deque<std::string> m_writeQueue;
    /// Writing in progress
    bool m_writing;
    /// On open callback
    WsServerOnOpen onOpen;
    /// On close callback
    WsServerOnClose onClose;
    /// On message callback
    WsServerOnMessage onMessage;
    /// On auth callback
    WsServerOnAuth onAuth;
    /// Authentication timeout
    const std::chrono::seconds m_authTimeout;
    /// Authentication timer
    boost::asio::steady_timer m_authTimer;
    /// Session is authenticated
    bool m_authenticated = false;
    /// Token expiration
    int64_t m_expiration = -1;
  public:
    WebsocketSession() = delete;

    /**
     * @brief Parameterized constructor
     *
     * Initializes the websocket session with specified parameters.
     * Constructor also retrieves client address and port for logging purposes.
     *
     * @param id Session ID
     * @param stream Session stream
     */
    WebsocketSession(std::size_t id, StreamType&& stream, uint16_t authTimeout)
    : m_id(id), m_stream(std::move(stream)),
      m_authTimeout(authTimeout), m_authTimer(m_stream.get_executor())
    {
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

    /**
     * @brief Destructor
     */
    virtual ~WebsocketSession() = default;

    /**
     * @brief Get session ID
     * @return `std::size_t` Session ID
     */
    std::size_t getId() const override {
      return m_id;
    };

    /**
     * @brief Get server host
     * @return `std::string&` Server host
     */
    const std::string& getAddress() const override {
      return m_address;
    };

    /**
     * @brief Get server port
     * @return `uint16_t` Server port
     */
    uint16_t getPort() const override {
      return m_port;
    };

    /**
     * @brief Schedules a message to be sent to client
     *
     * If session does not require authentication, the message is added to write queue.
     * If writing is not currently in progress, a write cycle is started.
     *
     * If session does require authentication, and is authenticated, the message is added to write queue,
     * otherwise the message queue is cleared, a session expiration message is sent to client,
     * the session is marked unauthenticated, and a new authentication timer is started.
     *
     * @param message Message to send
     */
    void send(const std::string& message) override {
      boost::asio::post(
        m_stream.get_executor(),
        [self = this->shared_from_this(), message]() mutable {
          if (self->onAuth) {
            if (!self->m_authenticated) {
              return;
            }

            auto now = DateTimeUtils::get_current_timestamp();
            if (now >= self->m_expiration) {
              self->m_authenticated = false;
              self->m_expiration = -1;
              self->m_writeQueue.clear();
              self->m_writing = false;
              self->m_writeQueue.push_back(
                create_auth_error_message(
                  make_error_code(auth_error::expired_token)
                )
              );
              self->write();
              self->init_auth_timeout();
              return;
            }
          }

          self->m_writeQueue.push_back(std::move(message));
          if (!self->m_writing) {
            self->write();
          }
        }
      );
    }

    /**
     * @brief Starts the session
     */
    void run() override {
      boost::asio::dispatch(
        m_stream.get_executor(),
        boost::beast::bind_front_handler(
          &WebsocketSession::on_run,
          this->shared_from_this()
        )
      );
    }

    /**
     * @brief Closes the session
     *
     * @param cc Close code
     */
    void close(boost::beast::websocket::close_code cc) override {
      boost::asio::post(
        m_stream.get_executor(),
        [self = this->shared_from_this(), cc]() {
          self->m_stream.async_close(
            cc,
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

    /**
     * @brief Sets onOpen callback
     */
    void setOnOpen(WsServerOnOpen onOpen) override {
      this->onOpen = onOpen;
    }

    /**
     * @brief Sets onClose callback
     */
    void setOnClose(WsServerOnClose onClose) override {
      this->onClose = onClose;
    }

    /**
     * @brief Sets onMessage callback
     */
    void setOnMessage(WsServerOnMessage onMessage) override {
      this->onMessage = onMessage;
    };

    /**
     * @brief Sets onAuth callback
     */
    void setOnAuth(WsServerOnAuth onAuth) override {
      this->onAuth = onAuth;
    };

    /**
     * Checks if session is authenticated
     * @return `true` if session is authenticated, `false` otherwise
     */
    bool isAuthenticated() override {
      return m_authenticated;
    }

  private:
    /**
     * @brief on_run callback
     *
     * If session uses TLS stream, performs TLS handshake with a client,
     * otherwise the session can accept client handshake immediately.
     */
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

    /**
     * @brief Handles TLS handshake result
     *
     * If TLS handshake succeeds, the client handshake request can be accepted,
     * otherwise the session is closed with protocol_error code.
     *
     * @param ec TLS handshake error code
     */
    void on_handshake(boost::beast::error_code ec) {
      if (ec) {
        TRC_WARNING(
          SESSION_LOG(m_id, m_address, m_port)
          << "Failed to complete handshake: "
          << BEAST_ERR_LOG(ec)
        );
        this->close(boost::beast::websocket::close_code::protocol_error);
        return;
      }
      this->accept();
    }

    /**
     * @brief Accepts client handshake request
     */
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

    /**
     * @brief Handles client handshake accept result
     *
     * If the accept call succeeded, the session can start reading messages from stream.
     * If `onOpen` callback has been passed to session object, it is executed before reading data from stream.
     * If `onAuth` callback has been passed to session object, authentication timeout is started before reading data from stream.
     *
     * @param ec Accept error code
     */
    void on_accept(boost::beast::error_code ec) {
      if (ec) {
        TRC_WARNING(
          SESSION_LOG(m_id, m_address, m_port)
          << "Failed to accept client connection: "
          << BEAST_ERR_LOG(ec)
        );
        this->close(boost::beast::websocket::close_code::protocol_error);
      } else {
        if (this->onOpen) {
          this->onOpen(m_id);
        }
        if (this->onAuth) {
          this->init_auth_timeout();
        }
        this->read();
      }
    }

    /**
     * @brief Begins reading from session stream.
     */
    void read() {
      m_stream.async_read(
        m_buffer,
        boost::beast::bind_front_handler(
          &WebsocketSession::on_read,
          this->shared_from_this()
        )
      );
    }

    /**
     * @brief Handles message read from session stream.
     *
     * If message has been successfully read, it is processed further.
     * If a frame close message is read, the session notifies server object and reading loop ends.
     * If an error has occurred while reading message, the session is closed from server side and reading loop ends.
     *
     * If session expects clients to authenticate (`onAuth` callback is passed),
     * the first received message should be an authentication message,
     * otherise the session will be closed with policy_error.
     *
     * Similarly, if a client fails to authenticate within specified time
     * or their authenticator is not valid, is expired or revoked,
     * the session will also be closed with policy error.
     *
     * @param ec Read operation error code
     * @param bytesRead Length of message data
     */
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
        this->close(boost::beast::websocket::close_code::internal_error);
        return;
      }

      auto now = DateTimeUtils::get_current_timestamp();
      std::string message = boost::beast::buffers_to_string(m_buffer.data());

      bool acceptMessage = false;

      if (this->onAuth) {
        // auth possible
        if (!m_authenticated) {
          // message arrived before timeout, cancel
          m_authTimer.cancel();
          // not authenticated, check for auth
          auto ec = this->auth(message);
          if (ec) {
            // not auth message or auth failed, close
            this->send_system(create_auth_error_message(ec));
            this->close(boost::beast::websocket::close_code::policy_error);
            return;
          }
          this->send(create_auth_success_message(m_expiration));
        } else {
          // authenticated
          if (m_expiration < now) {
            // token expired, message dropped
            auto ec = make_error_code(auth_error::expired_token);
            m_authenticated = false;
            m_expiration = -1;
            this->send_system(create_auth_error_message(ec));
            this->init_auth_timeout();
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

      m_buffer.consume(m_buffer.size());
      this->read();
    }

    /**
     * @brief Performs authentication
     *
     * The method checks whether incoming message is an authentication message.
     * If the message is correct, an authentication attempt will be performed,
     * otherwise the session is closed.
     *
     * On successful authentication, the session stores token expiration time for later use.
     *
     * If authentication fails for any reason, an authentication error message is sent.
     *
     * @param message Incoming message
     */
    boost::beast::error_code auth(const std::string& message) {
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

    /**
     * @brief Initializes and starts authentication timer.
     */
    void init_auth_timeout() {
      m_authTimer.expires_after(m_authTimeout);
      m_authTimer.async_wait(
        boost::beast::bind_front_handler(
          &WebsocketSession::on_auth_timeout,
          this->shared_from_this()
        )
      );
    }

    /**
     * @brief Authentication timeout handler
     *
     * If the timer was cancelled, no other action needs to be taken
     * as it is assumed that authentication timer is cancelled as a result
     * of successful authentication.
     *
     * If the timer was not cancelled, and the session is not authenticated,
     * an appropriate error message is sent to the client and the session is closed with policy_error.
     *
     * @param ec Operation error code
     */
    void on_auth_timeout(boost::beast::error_code ec) {
      if (ec == boost::asio::error::operation_aborted) {
        // timer cancelled, auth successful
        return;
      }

      if (!m_authenticated) {
        // session is not authenticated by the end of timeout
        this->send_system(create_auth_error_message(make_error_code(auth_error::auth_timeout)));
        this->close(boost::beast::websocket::close_code::policy_error);
      }
    }

    /**
     * @brief Sends system message to client
     *
     * This is an internal call that bypasses session authentication
     * to ensure that a client receives this message.
     *
     * This method should only be used to send messages informing
     * clients of errors related to authentication.
     *
     * @param message Message to send
     */
    void send_system(const std::string& message) {
      boost::asio::post(
        m_stream.get_executor(),
        [self = this->shared_from_this(), message]() mutable {
          // clear queue of stale messages
          self->m_writeQueue.clear();
          self->m_writing = false;
          // write system message
          self->m_writeQueue.push_back(std::move(message));
          if (!self->m_writing) {
            self->write();
          }
        }
      );
    }

    /**
     * @brief Pops the first message from queue and writes to stream.
     */
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

    /**
     * @brief Handles write operation results
     *
     * If the write operation was a success, the handler
     * will continue scheduling message writes until write queue is empty.
     *
     * If the write operation fails for whatever reason,
     * the write queue is emptied and writing is stopped.
     * The session is then closed with internal_error.
     *
     * @param ec Write operation error code
     * @param bytesWritten Length of written message data
     */
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
        this->close(boost::beast::websocket::close_code::internal_error);
      } else {
        m_writeQueue.pop_front();
        if (!m_writeQueue.empty()) {
          this->write();
        } else {
          m_writing = false;
        }
      }
    }

    /**
     * Performs proper transport shutdown on TLS and socket layer.
     */
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

    /**
     * @brief Attempts to notify the server of session close.
     *
     * If the onClose callback was not passed, this function has no effect.
     */
    void notify_server_close() {
      if (this->onClose) {
        this->onClose(m_id);
      }
    }
  };

}

