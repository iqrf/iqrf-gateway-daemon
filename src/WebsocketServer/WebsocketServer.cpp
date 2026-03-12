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

#include "Trace.h"
#include "WebsocketServer.h"
#include "WebSocketClientSession.h"

#include <atomic>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>

#include <boost/asio/ip/v6_only.hpp>
#include <rapidjson/pointer.h>
#include <openssl/ssl.h>

#define SERVER_LOG(instance, port) "[" << instance << ":" << port << "] "

namespace iqrf {

  class WebsocketServer::Impl {
  private:
    /// Websocket server parameters
    WebsocketServerParams wsParams_;
    /// On message handler
    WebSocketMessageHandler messageReceivedCallback_;
    /// On auth handler
    WebSocketAuthHandler authCallback_;
    /// On close handler
    WebSocketCloseHandler connectionClosedCallback_;
    /// Server address
    boost::asio::ip::address serverAddr_;
    /// Thread
    std::thread thread_;
    /// IO context
    std::optional<boost::asio::io_context> ioc_ = std::nullopt;
    /// SSL context
    std::optional<boost::asio::ssl::context> sslCtx_ = std::nullopt;
    /// TCP acceptor mutex
    std::mutex acceptorMtx_;
    /// TCP acceptor
    std::optional<boost::asio::ip::tcp::acceptor> acceptor_;
    /// IO context work guard
    std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> workGuard_;
    /// Session ID
    std::atomic<std::size_t> sessionIdCounter_{0};
    /// Session registry mutex
    std::mutex sessionMtx_;
    /// Session registry
    std::unordered_map<size_t, std::shared_ptr<IWebSocketClientSession>> sessionStorage_;
  public:
    Impl(const WebsocketServerParams& params): wsParams_(params) {
      TRC_FUNCTION_ENTER("");
      initialize();
      TRC_FUNCTION_LEAVE("");
    }

    Impl(
      const WebsocketServerParams& params,
      WebSocketMessageHandler onMessage,
      WebSocketAuthHandler onAuth,
      WebSocketCloseHandler onClose
    ):
      wsParams_(params),
      messageReceivedCallback_(onMessage),
      authCallback_(onAuth),
      connectionClosedCallback_(onClose)
    {
      TRC_FUNCTION_ENTER("");
      initialize();
      TRC_FUNCTION_LEAVE("");
    }

    ~Impl() {
      TRC_FUNCTION_ENTER("");
      // cleanup handlers
      messageReceivedCallback_ = nullptr;
      authCallback_ = nullptr;

      stop();
      sslCtx_.reset();
      TRC_FUNCTION_LEAVE("");
    }

    ///// Public API /////

    void start() {
      std::lock_guard<std::mutex> lock(acceptorMtx_);
      if (acceptor_.has_value() && acceptor_->is_open()) {
        // already running
        return;
      }

      // setup objects
      ioc_.emplace(1);
      workGuard_.emplace(boost::asio::make_work_guard(*ioc_));
      acceptor_.emplace(*ioc_);

      boost::asio::ip::tcp::endpoint endpoint(serverAddr_, wsParams_.port);
      boost::beast::error_code ec;
      acceptor_->open(endpoint.protocol(), ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Failed to open acceptor: " << BEAST_ERR_LOG(ec)
        );
      }

      acceptor_->set_option(boost::asio::ip::v6_only(false), ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Failed to disable IPv6 only connections." << BEAST_ERR_LOG(ec)
        )
      }

      acceptor_->set_option(boost::asio::socket_base::reuse_address(true), ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Failed to set address reuse: " << BEAST_ERR_LOG(ec)
        );
      }

      acceptor_->bind(endpoint, ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Failed to bind acceptor to the server address: " << BEAST_ERR_LOG(ec)
        );
      }

      acceptor_->listen(boost::asio::socket_base::max_listen_connections, ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Failed to start listening for connections: " << BEAST_ERR_LOG(ec)
        );
      }

      doAccept();

      // run IO context in a separate thread
      thread_ = std::thread([&]() {
        ioc_->run();
      });
    }

    bool isListening() {
      std::lock_guard<std::mutex> lock(acceptorMtx_);
      return acceptor_.has_value() && acceptor_->is_open();
    }

    void closeSession(const std::size_t sessionId, const boost::beast::websocket::close_code ec) {
      std::lock_guard<std::mutex> lock(sessionMtx_);
      auto record = sessionStorage_.find(sessionId);
      if (record != sessionStorage_.end()) {
        record->second->closeSession(ec);
      }
    }

    void stop() {
      std::lock_guard<std::mutex> lock(acceptorMtx_);

      // stop accepting new connections
      if (acceptor_.has_value()) {
        boost::beast::error_code ec;
        acceptor_->close(ec);
      }

      // kill existing sessions
      closeAllSessions();

      // stop event loop
      if (workGuard_.has_value()) {
        workGuard_->reset();
      }
      if (ioc_.has_value()) {
        ioc_->stop();
      }

      // join io context thread
      if (thread_.joinable()) {
        thread_.join();
      }

      // cleanup
      sessionStorage_.clear();
      acceptor_.reset();
      workGuard_.reset();
      ioc_.reset();
    }

    void send(const std::string& message) {
      std::lock_guard<std::mutex> lock(sessionMtx_);
      for (auto [_, session] : sessionStorage_) {
        session->sendMessage(message);
      }
    }

    void send(const std::size_t sessionId, const std::string& message) {
      std::lock_guard<std::mutex> lock(sessionMtx_);
      auto record = sessionStorage_.find(sessionId);
      if (record != sessionStorage_.end()) {
        record->second->sendMessage(message);
      } else {
        TRC_WARNING(
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Cannot send message, session ID "
          << sessionId << " not found"
        );
      }
    }

  private:
    boost::asio::ssl::context::options getSslContextOptions() {
      boost::asio::ssl::context::options options =
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::no_sslv3 |
        boost::asio::ssl::context::single_dh_use;
      switch (wsParams_.tlsMode) {
        case TlsModes::MODERN:
          options |=
            boost::asio::ssl::context::no_tlsv1 |
            boost::asio::ssl::context::no_tlsv1_1 |
            boost::asio::ssl::context::no_tlsv1_2;
          break;
        case TlsModes::INTERMEDIATE:
          options |=
            boost::asio::ssl::context::no_tlsv1 |
            boost::asio::ssl::context::no_tlsv1_1;
          break;
        case TlsModes::OLD:
          break;
      }
      return options;
    }

    std::string getSslCiphers() {
      std::string ciphers;
      switch (wsParams_.tlsMode) {
        case TlsModes::MODERN:
          break;
        case TlsModes::INTERMEDIATE:
            ciphers =
              "ECDHE-ECDSA-AES128-GCM-SHA256:"
              "ECDHE-RSA-AES128-GCM-SHA256:"
              "ECDHE-ECDSA-AES256-GCM-SHA384:"
              "ECDHE-RSA-AES256-GCM-SHA384:"
              "ECDHE-ECDSA-CHACHA20-POLY1305:"
              "ECDHE-RSA-CHACHA20-POLY1305:"
              "DHE-RSA-AES128-GCM-SHA256:"
              "DHE-RSA-AES256-GCM-SHA384:"
              "DHE-RSA-CHACHA20-POLY1305";
          break;
        case TlsModes::OLD:
            ciphers =
              "ECDHE-ECDSA-AES128-GCM-SHA256:"
              "ECDHE-RSA-AES128-GCM-SHA256:"
              "ECDHE-ECDSA-AES256-GCM-SHA384:"
              "ECDHE-RSA-AES256-GCM-SHA384:"
              "ECDHE-ECDSA-CHACHA20-POLY1305:"
              "ECDHE-RSA-CHACHA20-POLY1305:"
              "DHE-RSA-AES128-GCM-SHA256:"
              "DHE-RSA-AES256-GCM-SHA384:"
              "DHE-RSA-CHACHA20-POLY1305:"
              "ECDHE-ECDSA-AES128-SHA256:"
              "ECDHE-RSA-AES128-SHA256:"
              "ECDHE-ECDSA-AES128-SHA:"
              "ECDHE-RSA-AES128-SHA:"
              "ECDHE-ECDSA-AES256-SHA384:"
              "ECDHE-RSA-AES256-SHA384:"
              "ECDHE-ECDSA-AES256-SHA:"
              "ECDHE-RSA-AES256-SHA:"
              "DHE-RSA-AES128-SHA256:"
              "DHE-RSA-AES256-SHA256:"
              "AES128-GCM-SHA256:"
              "AES256-GCM-SHA384:"
              "AES128-SHA256:"
              "AES256-SHA256:"
              "AES128-SHA:"
              "AES256-SHA:"
              "DES-CBC3-SHA";
          break;
      }
      return ciphers;
    }

    void doAccept() {
      acceptor_->async_accept(
        boost::asio::make_strand(*ioc_),
        [this](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
          this->onAcceptCallback(std::move(ec), std::move(socket));
        }
      );
    }

    void onAcceptCallback(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
      // error accepting session
      if (ec == boost::asio::error::operation_aborted ||
          ec == boost::asio::error::bad_descriptor ||
          ec == boost::asio::error::not_socket
      ) {
        TRC_WARNING(
          SERVER_LOG(wsParams_.instance, wsParams_.port)
          << "Received non-recovery error code: "
          << BEAST_ERR_LOG(ec)
        );
        return;
      }

      if (ec) {
        TRC_WARNING(
          SERVER_LOG(wsParams_.instance, wsParams_.port)
          << "Failed to accept incoming connection: "
          << BEAST_ERR_LOG(ec)
        );
        doAccept();
        return;
      }

      // accept connection and create session as normal
      std::shared_ptr<IWebSocketClientSession> clientSession = nullptr;
      if (wsParams_.tls) {
        auto stream = TlsWebSocketStream(std::move(socket), *sslCtx_);
        clientSession = std::make_shared<WebSocketClientSession<TlsWebSocketStream>>(
          sessionIdCounter_++,
          std::move(stream),
          wsParams_.authTimeout
        );
      } else {
        auto stream = PlainWebSocketStream(std::move(socket));
        clientSession = std::make_shared<WebSocketClientSession<PlainWebSocketStream>>(
          sessionIdCounter_++,
          std::move(stream),
          wsParams_.authTimeout
        );
      }
      TRC_INFORMATION(
        SERVER_LOG(wsParams_.instance, wsParams_.port)
        << "Incoming connection from " << clientSession->getAddress() << ':' << clientSession->getPort()
        << ", session ID " << clientSession->getId()
      );
      // set callbacks
      clientSession->setOnMessage(messageReceivedCallback_);
      clientSession->setAuthCallback(authCallback_);
      clientSession->setOnClose(
        [this](std::size_t sessionId) {
          removeSession(sessionId);
        }
      );
      // store session in server
      addSession(clientSession);
      // run session
      clientSession->startSession();
      // accept another client connection
      doAccept();
    }

    void initialize() {
      if (wsParams_.localhostOnly) {
        serverAddr_ = boost::asio::ip::address_v6::loopback();
      } else {
        serverAddr_ = boost::asio::ip::address_v6::any();
      }
      if (wsParams_.tls) {
        initializeSsl();
      }
    }

    void initializeSsl() {
      sslCtx_.emplace(boost::asio::ssl::context::tls_server);
      sslCtx_->set_options(getSslContextOptions());
      if (wsParams_.certPath.size() == 0) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Certificate file not specified."
        );
      }
      if (!std::filesystem::exists(wsParams_.certPath)) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Certificate file does not exist."
        )
      }
      sslCtx_->use_certificate_chain_file(wsParams_.certPath);
      if (wsParams_.keyPath.size() == 0) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Private key file not specified."
        );
      }
      if (!std::filesystem::exists(wsParams_.keyPath)) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Private key file does not exist."
        )
      }
      sslCtx_->use_private_key_file(wsParams_.keyPath, boost::asio::ssl::context_base::file_format::pem);
      sslCtx_->set_verify_mode(boost::asio::ssl::verify_none);
      auto ciphers = getSslCiphers();
      if (!ciphers.empty() && SSL_CTX_set_cipher_list(sslCtx_->native_handle(), ciphers.c_str()) != 1) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Failed to configure SSL cipher suites."
        );
      }
    }

    void addSession(std::shared_ptr<IWebSocketClientSession> session) {
      std::lock_guard<std::mutex> lock(sessionMtx_);
      sessionStorage_[session->getId()] = session;

      TRC_INFORMATION(
        SERVER_LOG(wsParams_.instance, wsParams_.port)
        << "New client session registered with ID " << session->getId()
      );
    }

    void removeSession(const size_t sessionId) {
      std::lock_guard<std::mutex> lock(sessionMtx_);

      auto record = sessionStorage_.find(sessionId);
      if (record != sessionStorage_.end()) {
        if (connectionClosedCallback_) {
          connectionClosedCallback_(sessionId);
        }
        sessionStorage_.erase(sessionId);
        TRC_INFORMATION(
          SERVER_LOG(wsParams_.instance, wsParams_.port)
          << "Client session ID " << sessionId << " unregistered"
        );
      } else {
        TRC_WARNING(
          SERVER_LOG(wsParams_.instance, wsParams_.port)
          << "Cannot remove non-existent session with ID "
          << sessionId
        );
      }
    }

    void closeAllSessions() {
      std::lock_guard<std::mutex> lock(sessionMtx_);
      for (auto [_, session] : sessionStorage_) {
        session->closeSession(boost::beast::websocket::close_code::normal);
      }
    }
  };

  WebsocketServer::WebsocketServer(const WebsocketServerParams& params): impl_(std::make_unique<Impl>(params)) {}

  WebsocketServer::WebsocketServer(
    const WebsocketServerParams& params,
    WebSocketMessageHandler onMessage,
    WebSocketAuthHandler onAuth,
    WebSocketCloseHandler onClose
  ): impl_(std::make_unique<Impl>(params, onMessage, onAuth, onClose)) {}

  WebsocketServer::~WebsocketServer() = default;

  ///// Public API /////

  void WebsocketServer::start() {
    impl_->start();
  }

  bool WebsocketServer::isListening() {
    return impl_->isListening();
  }

  void WebsocketServer::closeSession(const std::size_t sessionId, const boost::beast::websocket::close_code ec) {
    impl_->closeSession(sessionId, ec);
  }

  void WebsocketServer::stop() {
    impl_->stop();
  }

  void WebsocketServer::send(const std::string& message) {
    impl_->send(message);
  }

  void WebsocketServer::send(const std::size_t sessionId, const std::string& message) {
    impl_->send(sessionId, message);
  }
}
