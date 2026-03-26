/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
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

#include "IWebSocketClientSession.h"
#include "Trace.h"
#include "WebsocketServer.h"
#include "TraceMacros.h"
#include "WebSocketClientSession.h"
#include "WebsocketServerParams.h"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/error.hpp>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

#include <boost/asio/ip/v6_only.hpp>
#include <rapidjson/pointer.h>
#include <openssl/ssl.h>
#include <utility>

#define SERVER_LOG(instance, port) "[" << instance << ":" << port << "] "

namespace iqrf {

  class WebsocketServer::Impl {
  private:
    struct Listener {
      std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor = nullptr;
      bool tls = false;
    };

    /// Websocket server parameters
    WebsocketServerParams wsParams_;
    /// On message handler
    WebSocketMessageHandler messageReceivedCallback_;
    /// On auth handler
    WebSocketAuthHandler authCallback_;
    /// On close handler
    WebSocketCloseHandler connectionClosedCallback_;
    /// Thread
    std::thread thread_;
    /// IO context
    std::optional<boost::asio::io_context> ioc_ = std::nullopt;
    /// SSL context
    std::optional<boost::asio::ssl::context> sslCtx_ = std::nullopt;
    /// TCP acceptor mutex
    std::mutex acceptorMtx_;
    /// TCP acceptor
    std::vector<Listener> listeners_;
    /// IO context work guard
    std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> workGuard_;
    /// Session manager
    SessionManager sessionManager_;
  public:
    Impl(const WebsocketServerParams& params): wsParams_(params), sessionManager_(wsParams_.maxClients) {
      TRC_FUNCTION_ENTER("");
      if (wsParams_.transportMode != TransportModes::PLAIN) {
        initializeSsl();
      }
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
      connectionClosedCallback_(onClose),
      sessionManager_(wsParams_.maxClients)
    {
      TRC_FUNCTION_ENTER("");
      if (wsParams_.transportMode != TransportModes::PLAIN) {
        initializeSsl();
      }
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
      if (!listeners_.empty()) {
        // already running
        return;
      }

      // setup objects
      ioc_.emplace(1);
      workGuard_.emplace(boost::asio::make_work_guard(*ioc_));

      // add listeners depending on params
      if (wsParams_.transportMode == TransportModes::PLAIN) {
        addListener(false, wsParams_.port, wsParams_.localhostOnly, false);
        addListener(true, wsParams_.port, wsParams_.localhostOnly, false);
      } else if (wsParams_.transportMode == TransportModes::TLS) {
        addListener(false, wsParams_.tlsPort, wsParams_.localhostOnly, true);
        addListener(true, wsParams_.tlsPort, wsParams_.localhostOnly, true);
      } else {
        addListener(false, wsParams_.port, wsParams_.localhostOnly, false);
        addListener(true, wsParams_.port, wsParams_.localhostOnly, false);
        addListener(false, wsParams_.tlsPort, wsParams_.localhostOnly, true);
        addListener(true, wsParams_.tlsPort, wsParams_.localhostOnly, true);
      }

      for (auto& listener : listeners_) {
        doAccept(listener);
      }

      // run IO context in a separate thread
      thread_ = std::thread([&]() {
        ioc_->run();
      });
    }

    void closeSession(const std::size_t sessionId, const boost::beast::websocket::close_code ec) {
      auto session = sessionManager_.getSession(sessionId);
      if (session) {
        session->closeSession(ec);
      }
    }

    void stop() {
      std::lock_guard<std::mutex> lock(acceptorMtx_);

      // stop accepting new connections
      boost::beast::error_code ec;
      for (auto& listener : listeners_) {
        listener.acceptor->close(ec);
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
      listeners_.clear();
      workGuard_.reset();
      ioc_.reset();
    }

    void send(const std::string& message) {
      sessionManager_.forEachSession(
        [message](std::shared_ptr<IWebSocketClientSession> session) {
          session->sendMessage(message);
        }
      );
    }

    void send(const std::size_t sessionId, const std::string& message) {
      auto session = sessionManager_.getSession(sessionId);
      if (session) {
        session->sendMessage(message);
      } else {
        TRC_WARNING(
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Cannot send message, session ID "
          << sessionId << " not found"
        );
      }
    }

  private:
    /**
     * Creates a boost asio ip adress object for binding from parameters
     *
     * @param ipv6 Use IPv6
     * @param localhost Localhost address
     * @return `boost::asio::ip::address` Boost asio address object for binding
     */
    boost::asio::ip::address makeAddress(bool ipv6, bool localhost) {
      if (ipv6) {
        return localhost
          ? boost::asio::ip::address_v6::loopback()
          : boost::asio::ip::address_v6::any();
      } else {
        return localhost
          ? boost::asio::ip::address_v4::loopback()
          : boost::asio::ip::address_v4::any();
      }
    }

    /**
     * Creates a new listener with acceptor and stores is in listeners vector for later use.
     *
     * A new listener object is created according to specified parameters and stored in listeners vector.
     * These listeners are then used to accept incoming connections.
     *
     * IPv6 is used, acceptor is configured to not steal connections from IPv4 ports.
     *
     * @param ipv6 Use IPv6
     * @param port Port number
     * @param localhost Use localhost address
     * @param tls Requires TLS
     */
    void addListener(bool ipv6, uint16_t port, bool localhost, bool tls) {
      boost::beast::error_code ec;

      auto address = makeAddress(ipv6, localhost);
      boost::asio::ip::tcp::endpoint endpoint(address, port);
      auto acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(*ioc_);

      // open acceptor
      acceptor->open(endpoint.protocol(), ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Failed to open acceptor: " << BEAST_ERR_LOG(ec)
        );
      }

      // disable ipv6 dual stack
      if (ipv6) {
        acceptor->set_option(boost::asio::ip::v6_only(true), ec);
        if (ec) {
          THROW_EXC_TRC_WAR(
            std::runtime_error,
            SERVER_LOG(wsParams_.instance, wsParams_.port) << "Failed to disable IPv6 only connections." << BEAST_ERR_LOG(ec)
          )
        }
      }

      // reuse address
      acceptor->set_option(boost::asio::socket_base::reuse_address(true), ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Failed to set address reuse: " << BEAST_ERR_LOG(ec)
        );
      }

      // bind to address
      acceptor->bind(endpoint, ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Failed to bind acceptor to the server address: " << BEAST_ERR_LOG(ec)
        );
      }

      // listen for connections
      acceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(wsParams_.instance, wsParams_.port) << "Failed to start listening for connections: " << BEAST_ERR_LOG(ec)
        );
      }

      listeners_.push_back(Listener{
        .acceptor = acceptor,
        .tls = tls
      });
    }

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

    void doAccept(Listener& listener) {
      listener.acceptor->async_accept(
        boost::asio::make_strand(*ioc_),
        [this, &listener](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
          // error accepting session
          if (ec == boost::asio::error::operation_aborted ||
              ec == boost::asio::error::bad_descriptor ||
              ec == boost::asio::error::not_socket
          ) {
            TRC_WARNING(
              SERVER_LOG(wsParams_.instance, wsParams_.port)
              << "Received non-recovery accept error code: "
              << BEAST_ERR_LOG(ec)
            );
            return;
          }
          onAcceptCallback(ec, std::move(socket), listener.tls);
          doAccept(listener);
        }
      );
    }

    void onAcceptCallback(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket, bool tls) {
      if (ec) {
        TRC_WARNING(
          SERVER_LOG(wsParams_.instance, wsParams_.port)
          << "Failed to accept incoming connection: "
          << BEAST_ERR_LOG(ec)
        );
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        socket.close();
        return;
      }

      // accept connection and create session as normal
      std::shared_ptr<IWebSocketClientSession> clientSession = nullptr;
      if (tls) {
        auto stream = std::make_shared<TlsWebSocketStream>(std::move(socket), *sslCtx_);
        clientSession = std::make_shared<WebSocketClientSession<TlsWebSocketStream>>(
          sessionManager_.getNextSessionId(),
          std::move(*stream),
          wsParams_.authTimeout,
          sessionManager_
        );
      } else {
        auto stream = std::make_shared<PlainWebSocketStream>(std::move(socket));
        clientSession = std::make_shared<WebSocketClientSession<PlainWebSocketStream>>(
          sessionManager_.getNextSessionId(),
          std::move(*stream),
          wsParams_.authTimeout,
          sessionManager_
        );
      }

      TRC_INFORMATION(
        SERVER_LOG(wsParams_.instance, wsParams_.port)
        << "Incoming connection from " << clientSession->getAddress() << ':' << clientSession->getPort()
        << ", session ID " << clientSession->getId()
      );

      // set callbacks
      clientSession->setOnOpen(
        [this](std::size_t sessionId) {
          TRC_INFORMATION(
            SERVER_LOG(wsParams_.instance, wsParams_.port)
            << "Client session ID " << sessionId << " registered"
          )
        }
      );
      clientSession->setOnMessage(messageReceivedCallback_);
      clientSession->setAuthCallback(authCallback_);
      clientSession->setOnClose(
        [this](std::size_t sessionId) {
          TRC_INFORMATION(
            SERVER_LOG(wsParams_.instance, wsParams_.port)
            << "Client session ID " << sessionId << " unregistered"
          );
        }
      );
      // run session
      clientSession->startSession();
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

    void closeAllSessions() {
      sessionManager_.forEachSession(
        [](std::shared_ptr<IWebSocketClientSession> session) {
          session->closeSession(boost::beast::websocket::close_code::normal);
        }
      );
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
