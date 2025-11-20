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
#include "WebsocketSession.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <iostream>
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
    WebsocketServerParams m_params;
    /// Server address
    boost::asio::ip::address m_address;
    /// Thread
    std::thread m_thread;
    /// IO context
    std::optional<boost::asio::io_context> m_ioc = std::nullopt;
    /// SSL context
    std::optional<boost::asio::ssl::context> m_ctx = std::nullopt;
    /// TCP acceptor mutex
    std::mutex m_acceptorMutex;
    /// TCP acceptor
    std::optional<boost::asio::ip::tcp::acceptor> m_acceptor;
    /// IO context work guard
    std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> m_workGuard;
    /// Session ID
    std::atomic<std::size_t> m_sessionCounter{0};
    /// Session registry mutex
    std::mutex m_sessionMutex;
    /// Session registry
    std::unordered_map<size_t, std::shared_ptr<IWebsocketSession>> m_sessionRegistry;
    /// On message handler
    WsServerOnMessage m_onMessage;
    /// On auth handler
    WsServerOnAuth m_onAuth;
    /// On close handler
    WsServerOnClose m_onClose;
  public:
    Impl(const WebsocketServerParams& params): m_params(params) {
      TRC_FUNCTION_ENTER("");
      initialize();
      TRC_FUNCTION_LEAVE("");
    }

    Impl(
      const WebsocketServerParams& params,
      WsServerOnMessage onMessage,
      WsServerOnAuth onAuth,
      WsServerOnClose onClose
    ): m_params(params), m_onMessage(onMessage), m_onAuth(onAuth), m_onClose(onClose) {
      TRC_FUNCTION_ENTER("");
      initialize();
      TRC_FUNCTION_LEAVE("");
    }

    ~Impl() {
      TRC_FUNCTION_ENTER("");
      stop();
      m_workGuard.reset();
      m_ioc->stop();
      if (m_thread.joinable()) {
        m_thread.join();
      }
      m_ctx.reset();
      m_ioc.reset();
      m_onMessage = nullptr;
      m_onAuth = nullptr;
      TRC_FUNCTION_LEAVE("");
    }

    ///// Public API /////

    void registerMessageHandler(WsServerOnMessage handler) {
      m_onMessage = handler;
    }

    void unregisterMessageHandler() {
      m_onMessage = nullptr;
    }

    void start() {
      std::lock_guard<std::mutex> lock(m_acceptorMutex);
      if (m_acceptor && m_acceptor->is_open()) {
        // already running
        return;
      }

      boost::asio::ip::tcp::endpoint endpoint(m_address, m_params.port);
      m_acceptor.emplace(boost::asio::make_strand(*m_ioc));

      boost::beast::error_code ec;
      m_acceptor->open(endpoint.protocol(), ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_params.instance, m_params.port) << "Failed to open acceptor: " << BEAST_ERR_LOG(ec)
        );
      }

      m_acceptor->set_option(boost::asio::ip::v6_only(false), ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_params.instance, m_params.port) << "Failed to disable IPv6 only connections." << BEAST_ERR_LOG(ec)
        )
      }

      m_acceptor->set_option(boost::asio::socket_base::reuse_address(true), ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_params.instance, m_params.port) << "Failed to set address reuse: " << BEAST_ERR_LOG(ec)
        );
      }

      m_acceptor->bind(endpoint, ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_params.instance, m_params.port) << "Failed to bind acceptor to the server address: " << BEAST_ERR_LOG(ec)
        );
      }

      m_acceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_params.instance, m_params.port) << "Failed to start listening for connections: " << BEAST_ERR_LOG(ec)
        );
      }

      accept();

      m_thread = std::thread([&]() {
        m_ioc->run();
      });
    }

    bool isListening() {
      std::lock_guard<std::mutex> lock(m_acceptorMutex);
      return m_acceptor.has_value() && m_acceptor->is_open();
    }

    void closeSession(const std::size_t sessionId, const boost::beast::websocket::close_code ec) {
      std::lock_guard<std::mutex> lock(m_acceptorMutex);
      auto record = m_sessionRegistry.find(sessionId);
      if (record != m_sessionRegistry.end()) {
        record->second->close(ec);
      }
    }

    void stop() {
      std::lock_guard<std::mutex> lock(m_acceptorMutex);
      m_acceptor.reset();
      clearSessions();
    }

    void send(const std::string& message) {
      std::lock_guard<std::mutex> lock(m_sessionMutex);
      for (auto [_, session] : m_sessionRegistry) {
        session->send(message);
      }
    }

    void send(const std::size_t sessionId, const std::string& message) {
      std::lock_guard<std::mutex> lock(m_sessionMutex);
      auto record = m_sessionRegistry.find(sessionId);
      if (record != m_sessionRegistry.end()) {
        record->second->send(message);
      } else {
        TRC_WARNING(SERVER_LOG(m_params.instance, m_params.port) << "Cannot send message, session ID " << sessionId << " not found");
      }
    }

  private:
    boost::asio::ssl::context::options getSslContextOptions() {
      boost::asio::ssl::context::options options =
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::no_sslv3 |
        boost::asio::ssl::context::single_dh_use;
      switch (m_params.tlsMode) {
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
      switch (m_params.tlsMode) {
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

    void accept() {
      m_acceptor->async_accept(
        boost::asio::make_strand(*m_ioc),
        [this](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
          this->on_accept(std::move(ec), std::move(socket));
        }
      );
    }

    void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
      if (ec) {
        TRC_WARNING(
          SERVER_LOG(m_params.instance, m_params.port)
          << "Failed to accept incoming connection: "
          << BEAST_ERR_LOG(ec)
        );
      } else {
        std::shared_ptr<IWebsocketSession> session = nullptr;
        if (m_params.tls) {
          auto stream = WsStreamTls(std::move(socket), *m_ctx);
          session = std::make_shared<WebsocketSession<WsStreamTls>>(
            m_sessionCounter++,
            std::move(stream)
          );
        } else {
          auto stream = WsStreamPlain(std::move(socket));
          session = std::make_shared<WebsocketSession<WsStreamPlain>>(
            m_sessionCounter++,
            std::move(stream)
          );
        }
        TRC_INFORMATION(
          SERVER_LOG(m_params.instance, m_params.port)
          << "Incoming connection from " << session->getAddress() << ':' << session->getPort()
          << ", session ID " << session->getId()
        );
        session->setOnOpen(
          [this, session](std::size_t sessionId) {
            (void) sessionId;
            registerSession(session);
          }
        );
        session->setOnClose(
          [this](std::size_t sessionId) {
            unregisterSession(sessionId);
          }
        );
        session->setOnMessage(m_onMessage);
        session->setOnAuth(m_onAuth);
        session->run();
      }
      accept();
    }

    void initialize() {
      if (m_params.localhostOnly) {
        m_address = boost::asio::ip::address_v6::loopback();
      } else {
        m_address = boost::asio::ip::address_v6::any();
      }
      initializeIoc();
      if (m_params.tls) {
        initializeSsl();
      }
    }

    void initializeIoc() {
      m_ioc.emplace(1);
      m_workGuard.emplace(boost::asio::make_work_guard(*m_ioc));
    }

    void initializeSsl() {
      m_ctx.emplace(boost::asio::ssl::context::tls_server);
      m_ctx->set_options(getSslContextOptions());
      if (m_params.certPath.size() == 0) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_params.instance, m_params.port) << "Certificate file not specified."
        );
      }
      if (!std::filesystem::exists(m_params.certPath)) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_params.instance, m_params.port) << "Certificate file does not exist."
        )
      }
      m_ctx->use_certificate_chain_file(m_params.certPath);
      if (m_params.keyPath.size() == 0) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_params.instance, m_params.port) << "Private key file not specified."
        );
      }
      if (!std::filesystem::exists(m_params.keyPath)) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_params.instance, m_params.port) << "Private key file does not exist."
        )
      }
      m_ctx->use_private_key_file(m_params.keyPath, boost::asio::ssl::context_base::file_format::pem);
      m_ctx->set_verify_mode(boost::asio::ssl::verify_none);
      auto ciphers = getSslCiphers();
      if (!ciphers.empty() && SSL_CTX_set_cipher_list(m_ctx->native_handle(), ciphers.c_str()) != 1) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_params.instance, m_params.port) << "Failed to configure SSL cipher suites."
        );
      }
    }

    void registerSession(std::shared_ptr<IWebsocketSession> session) {
      std::lock_guard<std::mutex> lock(m_sessionMutex);
      m_sessionRegistry[session->getId()] = session;
      TRC_INFORMATION(
        SERVER_LOG(m_params.instance, m_params.port)
        << "New client session registered with ID " << session->getId()
      );
    }

    void unregisterSession(const size_t sessionId) {
      std::lock_guard<std::mutex> lock(m_sessionMutex);
      auto record = m_sessionRegistry.find(sessionId);
      if (record != m_sessionRegistry.end()) {
        if (m_onClose) {
          m_onClose(sessionId);
        }
        m_sessionRegistry.erase(sessionId);
        TRC_INFORMATION(
          SERVER_LOG(m_params.instance, m_params.port)
          << "Client session ID " << sessionId << " unregistered"
        );
      } else {
        TRC_WARNING(
          SERVER_LOG(m_params.instance, m_params.port)
          << "Cannot remove non-existent session with ID "
          << sessionId
        );
      }
    }

    void clearSessions() {
      std::lock_guard<std::mutex> lock(m_sessionMutex);
      for (auto [_, session] : m_sessionRegistry) {
        session->close(boost::beast::websocket::close_code::normal);
      }
      m_sessionRegistry.clear();
    }
  };

  WebsocketServer::WebsocketServer(const WebsocketServerParams& params): impl_(std::make_unique<Impl>(params)) {}

  WebsocketServer::WebsocketServer(
    const WebsocketServerParams& params,
    WsServerOnMessage onMessage,
    WsServerOnAuth onAuth,
    WsServerOnClose onClose
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
