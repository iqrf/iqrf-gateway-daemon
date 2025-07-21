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

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "iqrf__WebsocketMessaging.hxx"

TRC_INIT_MODULE(iqrf::WebsocketMessaging)

namespace iqrf {

  WebsocketMessaging::WebsocketMessaging() {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  WebsocketMessaging::~WebsocketMessaging() {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  ///// Component lifecycle /////

  void WebsocketMessaging::activate(const shape::Properties *props) {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "WebsocketMessaging instance activate" << std::endl <<
      "******************************"
    );

    modify(props);

    m_wsServer->registerMessageHandler(
      [&](const std::size_t sessionId, const std::string& msg) -> int {
        return handleMessageFromWebsocket(sessionId, msg);
      }
    );

    TRC_FUNCTION_LEAVE("")
  }

  void WebsocketMessaging::modify(const shape::Properties *props) {
    std::string instanceName;

    props->getMemberAsString("instance", instanceName);
    props->getMemberAsBool("acceptAsyncMsg", m_acceptAsyncMsg);

    m_messagingInstance.instance = instanceName;
  }

  void WebsocketMessaging::deactivate() {
    TRC_FUNCTION_ENTER("");

    m_wsServer->unregisterMessageHandler();
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "WebsocketMessaging instance deactivate" << std::endl <<
      "******************************"
    );
    TRC_FUNCTION_LEAVE("")
  }

  ///// Public API /////

  void WebsocketMessaging::registerMessageHandler(MessageHandlerFunc handler) {
    TRC_FUNCTION_ENTER("");
    m_messageHandlerFunc = handler;
    TRC_FUNCTION_LEAVE("")
  }

  void WebsocketMessaging::unregisterMessageHandler() {
    TRC_FUNCTION_ENTER("");
    m_messageHandlerFunc = IMessagingService::MessageHandlerFunc();
    TRC_FUNCTION_LEAVE("")
  }

  void WebsocketMessaging::sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t>& msg) {
    TRC_FUNCTION_ENTER("");

    std::string message(msg.begin(), msg.end());

    if (!messaging.hasClientSession<std::size_t>()) {
      TRC_WARNING("Cannot send message via [" << messaging.to_string() << "]: Client session ID missing.");
      return;
    }

    m_wsServer->send(messaging.getClientSession<std::size_t>(), message);
    TRC_FUNCTION_LEAVE("")
  }

  bool WebsocketMessaging::acceptAsyncMsg() const {
    return m_acceptAsyncMsg;
  }

  const MessagingInstance& WebsocketMessaging::getMessagingInstance() const {
    return m_messagingInstance;
  }

  ///// Private methods /////

  int WebsocketMessaging::handleMessageFromWebsocket(const std::size_t sessionId, const std::string& msg) {
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

  ///// Interface management /////

  void WebsocketMessaging::attachInterface(iqrf::IWsServer* iface) {
    m_wsServer = iface;
  }

  void WebsocketMessaging::detachInterface(iqrf::IWsServer* iface) {
    if (m_wsServer == iface) {
      m_wsServer = nullptr;
    }
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
