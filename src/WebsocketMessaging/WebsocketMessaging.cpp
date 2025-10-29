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

#define IMessagingService_EXPORTS

#include "WebsocketMessaging.h"
#include "Trace.h"
#include <vector>
#include <algorithm>
#include "WebsocketServer.h"

#include <rapidjson/pointer.h>

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "iqrf__WebsocketMessaging.hxx"

TRC_INIT_MODULE(iqrf::WebsocketMessaging)

namespace iqrf {

  class WebsocketMessaging::Impl {
  public:
    Impl() {}

    ~Impl() {}

    ///// Component lifecycle /////

    void activate(const shape::Properties *props) {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "WebsocketMessaging instance activate" << std::endl <<
        "******************************"
      );

      modify(props);

      m_server = std::make_unique<WebsocketServer>(m_params);
      m_server->registerMessageHandler(
        [&](const std::size_t sessionId, const std::string& msg) -> int {
          return handleMessageFromWebsocket(sessionId, msg);
        }
      );
      m_server->start();

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props) {
      TRC_FUNCTION_ENTER("");

      const rapidjson::Document& doc = props->getAsJson();
      std::string instance = rapidjson::Pointer("/instance").Get(doc)->GetString();
      uint16_t port = static_cast<uint16_t>(rapidjson::Pointer("/port").Get(doc)->GetUint());
      m_acceptAsyncMsg = rapidjson::Pointer("/acceptAsyncMsg").Get(doc)->GetBool();
      bool acceptOnlyLocalhost = rapidjson::Pointer("/acceptOnlyLocalhost").Get(doc)->GetBool();
      bool tlsEnabled = rapidjson::Pointer("/tlsEnabled").Get(doc)->GetBool();
      TlsModes tlsMode = tlsModeFromValue(rapidjson::Pointer("/tlsMode").Get(doc)->GetUint());
      std::string certPath = getCertPath(rapidjson::Pointer("/cert").Get(doc)->GetString());
      std::string keyPath = getCertPath(rapidjson::Pointer("/privKey").Get(doc)->GetString());
      m_params = WebsocketServerParams(
        instance,
        port,
        acceptOnlyLocalhost,
        tlsEnabled,
        tlsMode,
        certPath,
        keyPath
      );

      m_messagingInstance.instance = instance;

      TRC_FUNCTION_LEAVE("");
    }

    void deactivate() {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "WebsocketMessaging instance deactivate" << std::endl <<
        "******************************"
      );

      m_server->unregisterMessageHandler();
      TRC_FUNCTION_LEAVE("")
    }

    ///// Public API /////

    void registerMessageHandler(MessageHandlerFunc handler) {
      TRC_FUNCTION_ENTER("");
      m_messageHandlerFunc = handler;
      TRC_FUNCTION_LEAVE("")
    }

    void unregisterMessageHandler() {
      TRC_FUNCTION_ENTER("");
      m_messageHandlerFunc = IMessagingService::MessageHandlerFunc();
      TRC_FUNCTION_LEAVE("")
    }

    void sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t>& msg) {
      TRC_FUNCTION_ENTER("");

      std::string message(msg.begin(), msg.end());

      if (!messaging.hasClientSession<std::size_t>()) {
        TRC_WARNING("Cannot send message via [" << messaging.to_string() << "]: Client session ID missing.");
        return;
      }

      m_server->send(messaging.getClientSession<std::size_t>(), message);
      TRC_FUNCTION_LEAVE("")
    }

    bool acceptAsyncMsg() const {
      return m_acceptAsyncMsg;
    }

    const MessagingInstance& getMessagingInstance() const {
      return m_messagingInstance;
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
    std::string getCertPath(const std::string& path) {
      if (path.size() == 0 || path.at(0) == '/') {
        return path;
      }
      return m_launchService->getConfigurationDir() + "/certs/" + path;
    }

    int handleMessageFromWebsocket(const std::size_t sessionId, const std::string& msg) {
      TRC_DEBUG(
        "==================================" << std::endl
        << "Received from Websocket: " << PAR(sessionId) << std::endl
        << MEM_HEX_CHAR(msg.data(), msg.size())
      );

      std::vector<uint8_t> message(msg.begin(), msg.end());

      if (m_messageHandlerFunc) {
        auto auxMessaging = m_messagingInstance;
        auxMessaging.setClientSession<std::size_t>(sessionId);
        m_messageHandlerFunc(auxMessaging, message);
      }

      return 0;
    }

    /// Launcher service
    shape::ILaunchService *m_launchService = nullptr;
    /// Websocket server parameters
    WebsocketServerParams m_params;
    /// Websocket server
    std::unique_ptr<WebsocketServer> m_server = nullptr;
    /// Handler for incoming messages
    IMessagingService::MessageHandlerFunc m_messageHandlerFunc;
    /// Accept asynchronous messages
    bool m_acceptAsyncMsg = false;
    /// Messaging instance
    MessagingInstance m_messagingInstance = MessagingInstance(MessagingType::WS);
  };

  WebsocketMessaging::WebsocketMessaging(): impl_(std::make_unique<Impl>()) {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  WebsocketMessaging::~WebsocketMessaging() {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  ///// Component lifecycle /////

  void WebsocketMessaging::activate(const shape::Properties *props) {
    impl_->activate(props);
  }

  void WebsocketMessaging::modify(const shape::Properties *props) {
    impl_->modify(props);
  }

  void WebsocketMessaging::deactivate() {
    impl_->deactivate();
  }

  ///// Public API /////

  void WebsocketMessaging::registerMessageHandler(MessageHandlerFunc handler) {
    impl_->registerMessageHandler(handler);
  }

  void WebsocketMessaging::unregisterMessageHandler() {
    impl_->unregisterMessageHandler();
  }

  void WebsocketMessaging::sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t>& msg) {
    impl_->sendMessage(messaging, msg);
  }

  bool WebsocketMessaging::acceptAsyncMsg() const {
    return impl_->acceptAsyncMsg();
  }

  const MessagingInstance& WebsocketMessaging::getMessagingInstance() const {
    return impl_->getMessagingInstance();
  }

  ///// Interface management /////

  void WebsocketMessaging::attachInterface(shape::ILaunchService *iface) {
    impl_->attachInterface(iface);
  }

  void WebsocketMessaging::detachInterface(shape::ILaunchService *iface) {
    impl_->detachInterface(iface);
  }

  void WebsocketMessaging::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void WebsocketMessaging::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }
}
