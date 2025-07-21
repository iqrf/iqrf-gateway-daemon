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

#include <rapidjson/pointer.h>
#include <openssl/ssl.h>

#include <filesystem>
#include <iostream>

#include "iqrf__WsServer.hxx"

#define SERVER_LOG(addr, port) "[" << addr << ":" << port << "] "

TRC_INIT_MODULE(iqrf::WsServer)

namespace iqrf {

  WsServer::WsServer() {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  WsServer::~WsServer() {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  ///// Component lifecycle /////

  void WsServer::activate(const shape::Properties *props) {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "WsServer instance activate" << std::endl <<
      "******************************"
    );
    modify(props);

    boost::asio::ip::tcp::endpoint endpoint(m_address, m_port);
    m_ioc.emplace(1);
    m_workGuard.emplace(boost::asio::make_work_guard(*m_ioc));
    m_ctx.emplace(boost::asio::ssl::context::tls_server);
    m_ctx->set_options(
      boost::asio::ssl::context::default_workarounds |
      boost::asio::ssl::context::no_sslv2 |
      boost::asio::ssl::context::no_sslv3 |
      boost::asio::ssl::context::no_tlsv1 |
      boost::asio::ssl::context::no_tlsv1_1 |
      boost::asio::ssl::context::single_dh_use
    );
    if (!m_certPath.has_value()) {
      THROW_EXC_TRC_WAR(
        std::runtime_error,
        SERVER_LOG(m_address, m_port) << "Certificate file not specified."
      );
    }
    if (!std::filesystem::exists(m_certPath.value())) {
      THROW_EXC_TRC_WAR(
        std::runtime_error,
        SERVER_LOG(m_address, m_port) << "Certificate file does not exist."
      )
    }
    m_ctx->use_certificate_chain_file(m_certPath.value());
    if (!m_keyPath.has_value()) {
      THROW_EXC_TRC_WAR(
        std::runtime_error,
        SERVER_LOG(m_address, m_port) << "Private key file not specified."
      );
    }
    if (!std::filesystem::exists(m_keyPath.value())) {
      THROW_EXC_TRC_WAR(
        std::runtime_error,
        SERVER_LOG(m_address, m_port) << "Private key file does not exist."
      )
    }
    m_ctx->use_private_key_file(*m_keyPath, boost::asio::ssl::context_base::file_format::pem);
    m_ctx->set_verify_mode(boost::asio::ssl::verify_none);
    m_acceptor.emplace(boost::asio::make_strand(*m_ioc));
    boost::beast::error_code ec;

    m_acceptor->open(endpoint.protocol(), ec);
    if (ec) {
      THROW_EXC_TRC_WAR(
        std::runtime_error,
        SERVER_LOG(m_address, m_port) << "Failed to open acceptor: " << BEAST_ERR_LOG(ec)
      );
    }

    m_acceptor->set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
      THROW_EXC_TRC_WAR(
        std::runtime_error,
        SERVER_LOG(m_address, m_port) << "Failed to set address reuse: " << BEAST_ERR_LOG(ec)
      );
    }

    m_acceptor->bind(endpoint, ec);
    if (ec) {
      THROW_EXC_TRC_WAR(
        std::runtime_error,
        SERVER_LOG(m_address, m_port) << "Failed to bind acceptor to the server address: " << BEAST_ERR_LOG(ec)
      );
    }

    m_acceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
      THROW_EXC_TRC_WAR(
        std::runtime_error,
        SERVER_LOG(m_address, m_port) << "Failed to start listening for connections: " << BEAST_ERR_LOG(ec)
      );
    }

    this->accept();

    m_thread = std::thread([&]() {
      m_ioc->run();
    });
    TRC_FUNCTION_LEAVE("");
  }

  void WsServer::modify(const shape::Properties *props) {
    TRC_FUNCTION_ENTER("");

    m_certDir = m_launchService->getConfigurationDir() + "/certs/";

    const rapidjson::Document& doc = props->getAsJson();
    m_port = static_cast<uint16_t>(rapidjson::Pointer("/port").Get(doc)->GetUint());
    std::string certPath = rapidjson::Pointer("/cert").Get(doc)->GetString();
    if (certPath.size() > 0) {
      m_certPath = m_certDir + certPath;
    }
    std::string keyPath = rapidjson::Pointer("/privKey").Get(doc)->GetString();
    if (keyPath.size() > 0) {
      m_keyPath = m_certDir + keyPath;
    }
    m_acceptOnlyLocalhost = rapidjson::Pointer("/acceptOnlyLocalhost").Get(doc)->GetBool();
    m_address = boost::asio::ip::make_address(
      m_acceptOnlyLocalhost ? std::string("127.0.0.1") : std::string("localhost")
    );

    TRC_FUNCTION_LEAVE("");
  }

  void WsServer::deactivate() {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "WsServer instance deactivate" << std::endl <<
      "******************************"
    );
    for (auto [_, session] : m_sessionRegistry) {
      session->close();
    }
    m_workGuard.reset();
    m_ioc->stop();
    if (m_thread.joinable()) {
      m_thread.join();
    }
    TRC_FUNCTION_LEAVE("");
  }

  ///// Public API /////

  void WsServer::registerMessageHandler(WsServerOnMessage handler) {
    m_onMessage = handler;
  }

  void WsServer::unregisterMessageHandler() {
    m_onMessage = nullptr;
  }

  void WsServer::send(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto [_, session] : m_sessionRegistry) {
      session->send(message);
    }
  }

  void WsServer::send(const std::size_t sessionId, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto record = m_sessionRegistry.find(sessionId);
    if (record != m_sessionRegistry.end()) {
      record->second->send(message);
    } else {
      TRC_WARNING(SERVER_LOG(m_address, m_port) << "Cannot send message, session ID " << sessionId << " not found");
    }
  }

  ///// Private methods /////

  void WsServer::accept() {
    m_acceptor->async_accept(
      boost::asio::make_strand(*m_ioc),
      boost::beast::bind_front_handler(
        &WsServer::on_accept,
        this
      )
    );
  }

  void WsServer::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
    if (ec) {
      TRC_WARNING(
        SERVER_LOG(m_address, m_port)
        << "Failed to accept incoming connection: "
        << BEAST_ERR_LOG(ec)
      );
    } else {
      auto session = std::make_shared<WsSession>(m_sessionCounter++, std::move(socket), *m_ctx);
      TRC_INFORMATION(
        SERVER_LOG(m_address, m_port)
        << "Incoming connection from " << session->getAddress() << ':' << session->getPort()
        << ", session ID " << session->getId()
      );
      session->onOpen = [this, session](std::size_t sessionId, boost::beast::error_code ec) {
        (void) sessionId;
        boost::ignore_unused(ec);
        this->registerSession(session);
      };
      session->onClose = [this](std::size_t sessionId, boost::beast::error_code ec) {
        boost::ignore_unused(ec);
        this->unregisterSession(sessionId);
      };
      session->onMessage = m_onMessage;
      session->run();
    }
    this->accept();
  }

  void WsServer::registerSession(std::shared_ptr<WsSession> session) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessionRegistry[session->getId()] = session;
    TRC_INFORMATION(
      SERVER_LOG(m_address, m_port)
      << "New client session registered with ID " << session->getId()
    );
  }

  void WsServer::unregisterSession(const size_t sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto record = m_sessionRegistry.find(sessionId);
    if (record != m_sessionRegistry.end()) {
      m_sessionRegistry.erase(sessionId);
      TRC_INFORMATION(
        SERVER_LOG(m_address, m_port)
        << "Client session ID " << sessionId << " unregistered"
      );
    } else {
      TRC_WARNING(
        SERVER_LOG(m_address, m_port)
        << "Cannot remove non-existent session with ID "
        << sessionId
      );
    }
  }

  ///// Interface management /////

  void WsServer::attachInterface(shape::ILaunchService *iface) {
    m_launchService = iface;
  }

  void WsServer::detachInterface(shape::ILaunchService *iface) {
    if (m_launchService == iface) {
      m_launchService = nullptr;
    }
  }

  void WsServer::attachInterface(shape::ITraceService *iface) {
    shape::Tracer::get().addTracerService(iface);
  }

  void WsServer::detachInterface(shape::ITraceService *iface) {
    shape::Tracer::get().removeTracerService(iface);
  }
}
