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

#include "IWebSocketClientSession.h"
#include "CryptoUtils.h"
#include "DateTimeUtils.h"
#include "WebsocketServerUtils.h"
#include "Trace.h"

#define SESSION_LOG(session, addr, port) "[" << session << "|" << addr << ":" << port << "] "
#define BEAST_ERR_LOG(ec) ec.message() << "(" << ec.category().name() << "|" << ec.value() << ")"

namespace iqrf {

  template <typename Stream>
  class WebSocketClientSession : public IWebSocketClientSession, public std::enable_shared_from_this<WebSocketClientSession<Stream>> {
  private:
    /// Session ID
    const std::size_t sessionId_;
    /// Stream
    Stream stream_;
    /// Boost ASIO stran
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    /// Authentication timer
    boost::asio::steady_timer authTimer_;
    /// Authentication timeout
    const std::chrono::seconds authTimeout_;
    /// IP address
    std::string address_;
    /// Port number
    uint16_t port_;
    /// Buffer for incoming messages
    boost::beast::flat_buffer readBuffer_;
    /// Writing in progress
    bool isWriting_;
    /// Queue for writing messages to client
    std::deque<std::string> writeQueue_;
    /// On connection open callback
    WebSocketOpenHandler connectionOpenCallback_;
    /// On message received callback
    WebSocketMessageHandler messageReceivedCallback_;
    /// Authentication callback
    WebSocketAuthHandler authCallback_;
    /// On connection close callback
    WebSocketCloseHandler connectionCloseCallback_;
    /// Session is authenticated
    bool isAuthenticated_ = false;
    /// Token expiration
    int64_t expirationTime_ = -1;
    /// Can use service mode
    bool service_ = false;

    /// Compile time TLS stream check
    static constexpr bool usesTlsStream = std::is_same_v<Stream, TlsWebSocketStream>;
  public:

    /**
     * Delete default constructor
     */
    WebSocketClientSession() = delete;

    /**
     * @brief Parameterized constructor
     *
     * Initializes the websocket session with specified parameters.
     * Constructor also retrieves client address and port for logging purposes.
     *
     * @param id Session ID
     * @param stream Session stream
     * @param authTimeout Authentication timeout
     */
    WebSocketClientSession(
      std::size_t id,
      Stream&& stream,
      uint16_t authTimeout
    ):
      sessionId_(id),
      stream_(std::move(stream)),
      strand_(
        boost::asio::make_strand(
          static_cast<boost::asio::io_context&>(
            boost::beast::get_lowest_layer(stream_).get_executor().context()
          )
        )
      ),
      authTimer_(strand_),
      authTimeout_(authTimeout)
    {
      // get client address and port depending on stream type
      if constexpr (usesTlsStream) {
        auto endpoint = stream_.next_layer().lowest_layer().remote_endpoint();
        address_ = endpoint.address().to_string();
        port_ = endpoint.port();
      } else {
        auto endpoint = stream_.next_layer().socket().remote_endpoint();
        address_ = endpoint.address().to_string();
        port_ = endpoint.port();
      }
    }

    /**
     * @brief Destructor
     */
    virtual ~WebSocketClientSession() = default;

    /**
     * @brief Get session ID
     * @return `std::size_t` Session ID
     */
    std::size_t getId() const override {
      return sessionId_;
    };

    /**
     * @brief Get server host
     * @return `std::string&` Server host
     */
    const std::string& getAddress() const override {
      return address_;
    };

    /**
     * @brief Get server port
     * @return `uint16_t` Server port
     */
    uint16_t getPort() const override {
      return port_;
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
    void sendMessage(std::string message) override {
      boost::asio::post(
        strand_,
        [self = this->shared_from_this(), message]() mutable {
          // need to check auth status
          if (self->authCallback_) {

            // not authenticated, exit
            if (!self->isAuthenticated_) {
              return;
            }

            // check if session expired
            auto now = DateTimeUtils::get_current_timestamp();
            if (now >= self->expirationTime_) {
              TRC_WARNING(
                SESSION_LOG(self->sessionId_, self->address_, self->port_)
                << "Session expired while processing request, closing session."
              );
              // mark unautenticated and reset expiration time
              self->isAuthenticated_ = false;
              self->expirationTime_ = -1;
              self->service_ = false;

              // stop writing and clear message queue
              self->isWriting_ = false;
              self->writeQueue_.clear();

              // send expired message
              self->writeQueue_.push_back(
                create_auth_error_message(
                  make_error_code(auth_error::expired_token)
                )
              );
              self->doWrite();
              self->closeSession(boost::beast::websocket::close_code::policy_error);
              return;
            }
          }

          // push message to write queue
          self->writeQueue_.push_back(std::move(message));
          // start writing cycle if not active
          if (!self->isWriting_) {
            self->doWrite();
          }
        }
      );
    }

    /**
     * @brief Starts the session
     */
    void startSession() override {
      boost::asio::dispatch(
        stream_.get_executor(),
        boost::asio::bind_executor(
          strand_,
          boost::beast::bind_front_handler(
            &WebSocketClientSession::runCallback,
            this->shared_from_this()
          )
        )
      );
    }

    /**
     * @brief Closes the session
     *
     * @param cc Close code
     */
    void closeSession(boost::beast::websocket::close_code cc) override {
      boost::asio::post(
        strand_,
        [self = this->shared_from_this(), cc]() {
          self->stream_.async_close(
            cc,
            boost::asio::bind_executor(
              self->strand_,
              [self](boost::beast::error_code ec) {
                if (ec) {
                  TRC_WARNING(
                    SESSION_LOG(self->sessionId_, self->address_, self->port_)
                      << "Failed to close WebSocket session gracefully: "
                      << BEAST_ERR_LOG(ec)
                  );
                } else {
                  TRC_INFORMATION(
                    SESSION_LOG(self->sessionId_, self->address_, self->port_)
                      << "WebSocket session closed gracefully."
                  );
                }
                self->shutdownSocket();
              }
            )
          );
        }
      );
    }

    /**
     * @brief Register on connection open callback
     * @param onOpen On connection open callback
     */
    void setOnOpen(WebSocketOpenHandler onOpen) override {
      connectionOpenCallback_= onOpen;
    }

    /**
     * @brief Register on message received callback
     * @param onMessage On message received callback
     */
    void setOnMessage(WebSocketMessageHandler onMessage) override {
      messageReceivedCallback_ = onMessage;
    };

    /**
     * @brief Registers authentication callback
     * @param onAuth Authentication callback
     */
    void setAuthCallback(WebSocketAuthHandler onAuth) override {
      authCallback_ = onAuth;
    };

    /**
     * @brief Registers on connection close callback
     * @param onClose On connection close callback
     */
    void setOnClose(WebSocketCloseHandler onClose) override {
      connectionCloseCallback_= onClose;
    }


    /**
     * Checks if session is authenticated
     * @return `true` if session is authenticated, `false` otherwise
     */
    bool isAuthenticated() override {
      return isAuthenticated_;
    }

  private:
    /**
     * @brief Run callback
     *
     * If session uses TLS stream, performs TLS handshake with a client,
     * otherwise the session can accept client handshake immediately.
     */
    void runCallback() {
      boost::beast::get_lowest_layer(stream_)
        .expires_after(std::chrono::seconds(30));

      if constexpr (usesTlsStream) {
        stream_.next_layer().async_handshake(
          boost::asio::ssl::stream_base::server,
          boost::asio::bind_executor(
            strand_,
            boost::beast::bind_front_handler(
              &WebSocketClientSession::handshakeCallback,
              this->shared_from_this()
            )
          )
        );
      } else {
        this->doAccept();
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
    void handshakeCallback(boost::beast::error_code ec) {
      // ensure callback is running on the same strand
      BOOST_ASSERT(strand_.running_in_this_thread());

      if (ec) {
        TRC_WARNING(
          SESSION_LOG(sessionId_, address_, port_)
          << "Failed to complete handshake: "
          << BEAST_ERR_LOG(ec)
        );
        this->closeSession(boost::beast::websocket::close_code::protocol_error);
        return;
      }

      this->doAccept();
    }

    /**
     * @brief Accepts client handshake request
     */
    void doAccept() {
      boost::beast::get_lowest_layer(stream_).expires_never();

      // set server options
      stream_.set_option(
        boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server)
      );
      stream_.set_option(
        boost::beast::websocket::stream_base::decorator(
          [](boost::beast::websocket::response_type& res) {
            res.set(
              boost::beast::http::field::server,
              "iqrf-gateway-daemon (" + std::string(BOOST_BEAST_VERSION_STRING) + ")"
            );
          }
        )
      );

      stream_.async_accept(
        boost::asio::bind_executor(
          strand_,
          boost::beast::bind_front_handler(
            &WebSocketClientSession::acceptCallback,
            this->shared_from_this()
          )
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
    void acceptCallback(boost::beast::error_code ec) {
      // ensure callback is running on the same strand
      BOOST_ASSERT(strand_.running_in_this_thread());

      if (ec) {
        TRC_WARNING(
          SESSION_LOG(sessionId_, address_, port_)
          << "Failed to accept client connection: "
          << BEAST_ERR_LOG(ec)
        );
        this->closeSession(boost::beast::websocket::close_code::protocol_error);
        return;
      }

      // session successfully connected, executed registered on connection open callback
      if (connectionOpenCallback_) {
        connectionOpenCallback_(sessionId_);
      }

      // if session should authenticate, start autentication timer
      if (authCallback_) {
        this->startAuthTimer();
      }

      // start reading loop
      this->doRead();
    }

    /**
     * @brief Begins reading from session stream.
     */
    void doRead() {
      stream_.async_read(
        readBuffer_,
        boost::asio::bind_executor(
          strand_,
          boost::beast::bind_front_handler(
            &WebSocketClientSession::readCallback,
            this->shared_from_this()
          )
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
     * If session expects clients to authenticate,
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
    void readCallback(boost::beast::error_code ec, std::size_t bytesRead) {
      // ensure callback is running on the same strand
      BOOST_ASSERT(strand_.running_in_this_thread());

      boost::ignore_unused(bytesRead);

      // if connection is already closed, execute on connection close callback
      if (isRemoteCloseError(ec)) {
        TRC_INFORMATION(
          SESSION_LOG(sessionId_, address_, port_)
          << "Connection closed: "
          << BEAST_ERR_LOG(ec)
        );
        // notify server close may be called twice in some scenarios
        // could store a variable which would check if onClose was already closed
        this->executeServerCloseCallback();
        return;
      }

      // if session was closed from this side, do nothing
      if (ec == boost::asio::error::operation_aborted) {
        return;
      }

      // if an error has occurred on this side, close
      if (ec) {
        TRC_WARNING(
          SESSION_LOG(sessionId_, address_, port_)
          << "Failed to read incoming message from stream: "
          << BEAST_ERR_LOG(ec)
        );
        this->closeSession(boost::beast::websocket::close_code::internal_error);
        return;
      }

      // get current timestamp for expiration check
      auto now = DateTimeUtils::get_current_timestamp();
      // read message from buffer and clear it before next message is read
      std::string message = boost::beast::buffers_to_string(readBuffer_.data());
      readBuffer_.consume(readBuffer_.size());

      bool acceptMessage = false;

      // if client should be authenticated
      if (authCallback_) {
        // session is not authenticated
        if (!isAuthenticated_) {
          authTimer_.cancel();

          auto ec = this->authenticate(message);
          // authentication failed, message contains invalid token or is not authentication message
          if (ec) {
            this->send_system(create_auth_error_message(ec));
            this->closeSession(boost::beast::websocket::close_code::policy_error);
            return;
          }

          this->sendMessage(create_auth_success_message(expirationTime_, service_));
        } else {
          // authenticated
          if (expirationTime_ < now) {
            TRC_WARNING(
              SESSION_LOG(sessionId_, address_, port_)
              << "Session has expired, dropping message and closing session."
            );
            // token expired, message dropped
            auto ec = make_error_code(auth_error::expired_token);
            isAuthenticated_ = false;
            expirationTime_ = -1;
            service_ = false;
            this->send_system(create_auth_error_message(ec));
            this->closeSession(boost::beast::websocket::close_code::policy_error);
            return;
          } else {
            // not expired, message accepted
            acceptMessage = true;
          }
        }
      } else {
        acceptMessage = true;
      }

      if (acceptMessage && messageReceivedCallback_) {
        messageReceivedCallback_(sessionId_, message);
      }

      this->doRead();
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
    boost::beast::error_code authenticate(const std::string& message) {
      nlohmann::json doc;
      try {
        doc = nlohmann::json::parse(message);
      } catch (const std::exception &e) {
        TRC_WARNING("Invalid JSON messsage received.")
        return make_error_code(auth_error::auth_failed);
      }
      auto isAuth = is_auth_message(doc);
      if (!isAuth) {
        return make_error_code(auth_error::no_auth);
      }

      auto token = doc["token"].get<std::string>();
      uint32_t tokenId;
      std::string secret;
      try {
        CryptoUtils::parse_token(token, tokenId, secret);
      } catch (const std::exception &e) {
        TRC_WARNING(
          SESSION_LOG(sessionId_, address_, port_)
          << "Failed to parse API token: "
          << e.what()
        );
        return make_error_code(auth_error::invalid_token);
      }

      // try to authenticate via passed callback and get expiration time
      int64_t tokenExpiration = 0;
      bool service = false;
      auto ec = authCallback_(sessionId_, tokenId, secret, tokenExpiration, service);
      if (ec) {
        TRC_WARNING(
          SESSION_LOG(sessionId_, address_, port_)
          << "Failed to authenticate session using token ID " << tokenId << ": "
          << BEAST_ERR_LOG(ec)
        );
      } else {
        TRC_INFORMATION(
          SESSION_LOG(sessionId_, address_, port_)
          << "Session successfully authenticated using token ID " << tokenId << "."
        );
        isAuthenticated_ = true;
        expirationTime_ = tokenExpiration;
        service_ = service;
      }
      return ec;
    }

    /**
     * @brief Starts authentication timer.
     */
    void startAuthTimer() {
      authTimer_.expires_after(authTimeout_);
      authTimer_.async_wait(
        boost::asio::bind_executor(
          strand_,
          boost::beast::bind_front_handler(
            &WebSocketClientSession::authTimeoutCallback,
            this->shared_from_this()
          )
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
    void authTimeoutCallback(boost::beast::error_code ec) {
      // ensure callback runs in the same strand
      BOOST_ASSERT(strand_.running_in_this_thread());

      // if timer was cancelled, do nothing
      if (ec == boost::asio::error::operation_aborted) {
        return;
      }

      // if timer ran down and session is not authenticated, close
      if (!isAuthenticated_) {
        TRC_WARNING(
          SESSION_LOG(sessionId_, address_, port_)
          << "Authentication timed out, closing session."
        );
        this->send_system(create_auth_error_message(make_error_code(auth_error::auth_timeout)));
        this->closeSession(boost::beast::websocket::close_code::policy_error);
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
    void send_system(std::string message) {
      boost::asio::post(
        strand_,
        [self = this->shared_from_this(), message]() mutable {
          // drop all otheer messages and send system message
          self->isWriting_ = false;
          self->writeQueue_.clear();

          self->writeQueue_.push_back(std::move(message));
          if (!self->isWriting_) {
            self->doWrite();
          }
        }
      );
    }

    /**
     * @brief Pops the first message from queue and writes to stream.
     */
    void doWrite() {
      isWriting_ = true;
      stream_.async_write(
        boost::asio::buffer(writeQueue_.front()),
        boost::asio::bind_executor(
          strand_,
          boost::beast::bind_front_handler(
            &WebSocketClientSession::writeCallback,
            this->shared_from_this()
          )
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
    void writeCallback(boost::beast::error_code ec, std::size_t bytesWritten) {
      // ensure callback runs in the same strand
      BOOST_ASSERT(strand_.running_in_this_thread());

      boost::ignore_unused(bytesWritten);

      // error writing message, clear queue and close
      if (ec) {
        TRC_WARNING(
          SESSION_LOG(sessionId_, address_, port_)
          << "Failed to write message to stream: "
          << BEAST_ERR_LOG(ec)
        );

        isWriting_ = false;
        writeQueue_.clear();
        this->closeSession(boost::beast::websocket::close_code::internal_error);
        return;
      }

      // write until queue is empty
      writeQueue_.pop_front();
      if (!writeQueue_.empty()) {
        this->doWrite();
      } else {
        isWriting_ = false;
      }
    }

    /**
     * @brief Shuts down and closes underlying socket of stream.
     */
    void shutdownSocket() {
      // shutdown and close socket depending on stream type
      if constexpr (usesTlsStream) {
        stream_.next_layer().async_shutdown(
          boost::asio::bind_executor(
            strand_,
            [self = this->shared_from_this()](boost::system::error_code tls_ec) {
              // ensure callback runs on the same strand
              BOOST_ASSERT(self->strand_.running_in_this_thread());
              boost::ignore_unused(tls_ec);

              boost::beast::error_code ec;
              self->stream_.next_layer().lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
              self->stream_.next_layer().lowest_layer().close(ec);
              self->executeServerCloseCallback();
            }
          )
        );
      } else {
        BOOST_ASSERT(strand_.running_in_this_thread());

        boost::beast::error_code ec;
        stream_.next_layer().socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        stream_.next_layer().socket().close(ec);
        this->executeServerCloseCallback();
      }
    }

    /**
     * @brief Attempt to execute on connection close callback
     *
     * If the callback was not passed, this function has no effect.
     */
    void executeServerCloseCallback() {
      if (connectionCloseCallback_) {
        connectionCloseCallback_(sessionId_);
      }
    }

    /**
     * @brief Checks if received error code is a close frame from the client side
     *
     * This function received error code to see if it is a should be
     * considered a normal or graceful shutdown. This includes both plain and TLS connections.
     *
     * Since graceful TLS connection requires a specific steps that all clients may not
     * always take, we should check for incorrectly closed TLS streams too.
     *
     * @param ec Error code
     */
    bool isRemoteCloseError(const boost::beast::error_code& ec) {
      // Graceful close or client not connected anymore, which should be okay too
      if (
        ec == boost::beast::websocket::error::closed ||
        ec == boost::asio::error::not_connected
      ) {
        return true;
      }

      // Check for incorrect TLS close that should be handled as normal
      if constexpr (usesTlsStream) {
        if (ec == boost::asio::ssl::error::stream_truncated) {
          return true;
        }
      }
      return false;
    }
  };

}

