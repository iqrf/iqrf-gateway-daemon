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
#include "WsServer.h"
#include "WsSession.h"

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

#include "iqrf__WsServer.hxx"

#define SERVER_LOG(instance, port) "[" << instance << ":" << port << "] "

TRC_INIT_MODULE(iqrf::WsServer)

namespace iqrf {

  class WsServer::Impl {
  public:
    Impl() {}

    ~Impl() {}

    ///// Component lifecycle /////

    void activate(const shape::Properties *props) {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "WsServer instance activate" << std::endl <<
        "******************************"
      );
      modify(props);
      initializeIoc();
      initializeSsl();
      TRC_FUNCTION_LEAVE("");
    }

    void modify(const shape::Properties *props) {
      TRC_FUNCTION_ENTER("");

      m_certDir = m_launchService->getConfigurationDir() + "/certs/";

      const rapidjson::Document& doc = props->getAsJson();
      m_instance = rapidjson::Pointer("/instance").Get(doc)->GetString();
      m_port = static_cast<uint16_t>(rapidjson::Pointer("/port").Get(doc)->GetUint());
      m_acceptOnlyLocalhost = rapidjson::Pointer("/acceptOnlyLocalhost").Get(doc)->GetBool();
      if (m_acceptOnlyLocalhost) {
        m_address = boost::asio::ip::address_v6::loopback();
      } else {
        m_address = boost::asio::ip::address_v6::any();
      }
      std::string certPath = rapidjson::Pointer("/cert").Get(doc)->GetString();
      m_certPath = getCertPath(certPath);
      std::string keyPath = rapidjson::Pointer("/privKey").Get(doc)->GetString();
      m_keyPath = getCertPath(keyPath);
      auto tlsMode = rapidjson::Pointer("/tlsMode").Get(doc)->GetUint();
      m_tlsMode = parseTlsModeValue(tlsMode);

      TRC_FUNCTION_LEAVE("");
    }

    void deactivate() {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "WsServer instance deactivate" << std::endl <<
        "******************************"
      );
      stop();
      m_workGuard.reset();
      m_ioc->stop();
      if (m_thread.joinable()) {
        m_thread.join();
      }
      m_ctx.reset();
      m_ioc.reset();
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

      boost::asio::ip::tcp::endpoint endpoint(m_address, m_port);
      m_acceptor.emplace(boost::asio::make_strand(*m_ioc));

      boost::beast::error_code ec;
      m_acceptor->open(endpoint.protocol(), ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_instance, m_port) << "Failed to open acceptor: " << BEAST_ERR_LOG(ec)
        );
      }

      m_acceptor->set_option(boost::asio::ip::v6_only(false), ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_instance, m_port) << "Failed to disable IPv6 only connections." << BEAST_ERR_LOG(ec)
        )
      }

      m_acceptor->set_option(boost::asio::socket_base::reuse_address(true), ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_instance, m_port) << "Failed to set address reuse: " << BEAST_ERR_LOG(ec)
        );
      }

      m_acceptor->bind(endpoint, ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_instance, m_port) << "Failed to bind acceptor to the server address: " << BEAST_ERR_LOG(ec)
        );
      }

      m_acceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
      if (ec) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_instance, m_port) << "Failed to start listening for connections: " << BEAST_ERR_LOG(ec)
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
        TRC_WARNING(SERVER_LOG(m_instance, m_port) << "Cannot send message, session ID " << sessionId << " not found");
      }
    }

    ///// Interface management /////

    void attachInterface(shape::ILaunchService *iface) {
      m_launchService = iface;
    }

    void detachInterface(shape::ILaunchService *iface) {
      if (m_launchService == iface) {
        m_launchService = nullptr;
      }
    }

  private:
    enum class TlsModes {
      MODERN = 0,
      INTERMEDIATE = 1,
      OLD = 2,
    };

    TlsModes parseTlsModeValue(unsigned int value) {
      switch (value) {
        case 0:
          return TlsModes::MODERN;
        case 1:
          return TlsModes::INTERMEDIATE;
        case 2:
          return TlsModes::OLD;
        default:
          THROW_EXC_TRC_WAR(
            std::runtime_error,
            SERVER_LOG(m_instance, m_port) << "Unknown or unsupported TLS mode value."
          );
      }
    }

    boost::asio::ssl::context::options getSslContextOptions() {
      boost::asio::ssl::context::options options =
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::no_sslv3 |
        boost::asio::ssl::context::single_dh_use;
      switch (m_tlsMode) {
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
      switch (m_tlsMode) {
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
          SERVER_LOG(m_instance, m_port)
          << "Failed to accept incoming connection: "
          << BEAST_ERR_LOG(ec)
        );
      } else {
        auto session = std::make_shared<WsSession>(m_sessionCounter++, std::move(socket), *m_ctx);
        TRC_INFORMATION(
          SERVER_LOG(m_instance, m_port)
          << "Incoming connection from " << session->getAddress() << ':' << session->getPort()
          << ", session ID " << session->getId()
        );
        session->onOpen = [this, session](std::size_t sessionId, boost::beast::error_code ec) {
          (void) sessionId;
          boost::ignore_unused(ec);
          registerSession(session);
        };
        session->onClose = [this](std::size_t sessionId, boost::beast::error_code ec) {
          boost::ignore_unused(ec);
          unregisterSession(sessionId);
        };
        session->onMessage = m_onMessage;
        session->run();
      }
      accept();
    }

    void initializeIoc() {
      m_ioc.emplace(1);
      m_workGuard.emplace(boost::asio::make_work_guard(*m_ioc));
    }

    void initializeSsl() {
      m_ctx.emplace(boost::asio::ssl::context::tls_server);
      m_ctx->set_options(getSslContextOptions());
      if (!m_certPath.has_value()) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_instance, m_port) << "Certificate file not specified."
        );
      }
      if (!std::filesystem::exists(m_certPath.value())) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_instance, m_port) << "Certificate file does not exist."
        )
      }
      m_ctx->use_certificate_chain_file(m_certPath.value());
      if (!m_keyPath.has_value()) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_instance, m_port) << "Private key file not specified."
        );
      }
      if (!std::filesystem::exists(m_keyPath.value())) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_instance, m_port) << "Private key file does not exist."
        )
      }
      m_ctx->use_private_key_file(*m_keyPath, boost::asio::ssl::context_base::file_format::pem);
      m_ctx->set_verify_mode(boost::asio::ssl::verify_none);
      auto ciphers = getSslCiphers();
      if (!ciphers.empty() && SSL_CTX_set_cipher_list(m_ctx->native_handle(), ciphers.c_str()) != 1) {
        THROW_EXC_TRC_WAR(
          std::runtime_error,
          SERVER_LOG(m_instance, m_port) << "Failed to configure SSL cipher suites."
        );
      }
    }

    void registerSession(std::shared_ptr<WsSession> session) {
      std::lock_guard<std::mutex> lock(m_sessionMutex);
      m_sessionRegistry[session->getId()] = session;
      TRC_INFORMATION(
        SERVER_LOG(m_instance, m_port)
        << "New client session registered with ID " << session->getId()
      );
    }

    void unregisterSession(const size_t sessionId) {
      std::lock_guard<std::mutex> lock(m_sessionMutex);
      auto record = m_sessionRegistry.find(sessionId);
      if (record != m_sessionRegistry.end()) {
        m_sessionRegistry.erase(sessionId);
        TRC_INFORMATION(
          SERVER_LOG(m_instance, m_port)
          << "Client session ID " << sessionId << " unregistered"
        );
      } else {
        TRC_WARNING(
          SERVER_LOG(m_instance, m_port)
          << "Cannot remove non-existent session with ID "
          << sessionId
        );
      }
    }

    void clearSessions() {
      std::lock_guard<std::mutex> lock(m_sessionMutex);
      for (auto [_, session] : m_sessionRegistry) {
        session->close();
      }
      m_sessionRegistry.clear();
    }

    std::string getCertPath(const std::string& path) {
      if (path.size() == 0 || path.at(0) == '/') {
        return path;
      }
      m_launchService->getConfigurationDir() + "/certs/" + path;
    }

    /// Launcher service
    shape::ILaunchService *m_launchService = nullptr;
    /// Instance name
    std::string m_instance;
    /// Path to directory with certificates
    std::string m_certDir;
    /// Server address
    boost::asio::ip::address m_address;
    /// Server port
    uint16_t m_port;
    /// Accept only localhost connections
    bool m_acceptOnlyLocalhost;
    /// Path to certificate file
    std::optional<std::string> m_certPath;
    /// Path to private key file
    std::optional<std::string> m_keyPath;
    /// TLS mode (modern, intermediate, old)
    TlsModes m_tlsMode;
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
    std::unordered_map<size_t, std::shared_ptr<WsSession>> m_sessionRegistry;
    /// On message handler
    WsServerOnMessage m_onMessage;
  };

  WsServer::WsServer(): impl_(std::make_unique<Impl>()) {}

  WsServer::~WsServer() = default;

  ///// Component lifecycle /////

  void WsServer::activate(const shape::Properties *props) {
    impl_->activate(props);
  }

  void WsServer::modify(const shape::Properties *props) {
    impl_->modify(props);
  }

  void WsServer::deactivate() {
    impl_->deactivate();
  }

  ///// Public API /////

  void WsServer::registerMessageHandler(WsServerOnMessage handler) {
    impl_->registerMessageHandler(handler);
  }

  void WsServer::unregisterMessageHandler() {
    impl_->unregisterMessageHandler();
  }

  void WsServer::start() {
    impl_->start();
  }

  bool WsServer::isListening() {
    return impl_->isListening();
  }

  void WsServer::stop() {
    impl_->stop();
  }

  void WsServer::send(const std::string& message) {
    impl_->send(message);
  }

  void WsServer::send(const std::size_t sessionId, const std::string& message) {
    impl_->send(sessionId, message);
  }

  ///// Interface management /////

  void WsServer::attachInterface(shape::ILaunchService *iface) {
    impl_->attachInterface(iface);
  }

  void WsServer::detachInterface(shape::ILaunchService *iface) {
    impl_->detachInterface(iface);
  }

  void WsServer::attachInterface(shape::ITraceService *iface) {
    shape::Tracer::get().addTracerService(iface);
  }

  void WsServer::detachInterface(shape::ITraceService *iface) {
    shape::Tracer::get().removeTracerService(iface);
  }
}
