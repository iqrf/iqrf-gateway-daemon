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

const unsigned IQRF_MQ_BUFFER_SIZE = 64 * 1024;

namespace iqrf {

  ////////////////////////////////////
  // class WebsocketMessaging::Imp
  ////////////////////////////////////
  class WebsocketMessaging::Imp
  {
  private:
    shape::IWebsocketService* m_iWebsocketService = nullptr;

    typedef std::pair<MessagingInstance, std::vector<uint8_t>> ConnMsg;

    TaskQueue<ConnMsg>* m_toMqMessageQueue = nullptr;

    bool m_acceptAsyncMsg = false;
    MessagingInstance m_messagingInstance = MessagingInstance(MessagingType::WS);
    IMessagingService::MessageHandlerFunc m_messageHandlerFunc;

    int handleMessageFromWebsocket(const std::vector<uint8_t>& message, const std::string& connId)
    {
      TRC_DEBUG("==================================" << std::endl <<
        "Received from Websocket: " << PAR(connId) << std::endl << MEM_HEX_CHAR(message.data(), message.size()));

      if (m_messageHandlerFunc) {
        auto auxMessaging = m_messagingInstance;
        auxMessaging.instance += '/' + connId;
        m_messageHandlerFunc(auxMessaging, message);
      }

      return 0;
    }

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    void registerMessageHandler(MessageHandlerFunc hndl)
    {
      TRC_FUNCTION_ENTER("");
      m_messageHandlerFunc = hndl;
      TRC_FUNCTION_LEAVE("")
    }

    void unregisterMessageHandler()
    {
      TRC_FUNCTION_ENTER("");
      m_messageHandlerFunc = IMessagingService::MessageHandlerFunc();
      TRC_FUNCTION_LEAVE("")
    }

    void sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t> & msg)
    {
      TRC_FUNCTION_ENTER("");
      TRC_DEBUG(MEM_HEX_CHAR(msg.data(), msg.size()));
      m_toMqMessageQueue->pushToQueue(std::make_pair(messaging, std::vector<uint8_t>(msg.data(), msg.data() + msg.size())));
      TRC_FUNCTION_LEAVE("")
    }

    bool acceptAsyncMsg() const {
      return m_acceptAsyncMsg;
    }

    const MessagingInstance &getMessagingInstance() const
    {
      return m_messagingInstance;
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "WebsocketMessaging instance activate" << std::endl <<
        "******************************"
      );

      std::string instanceName;

      props->getMemberAsString("instance", instanceName);
      props->getMemberAsBool("acceptAsyncMsg", m_acceptAsyncMsg);

      m_messagingInstance.instance = instanceName;

      m_toMqMessageQueue = shape_new TaskQueue<ConnMsg>([&](ConnMsg conMsg) {
        std::string messagingId2(conMsg.first.instance);
        std::string connId;
        if (std::string::npos != messagingId2.find_first_of('/')) {
          //preparse messageId to remove possible optional appended topic
          //we need just clean massaging name to find in map
          std::string buf(messagingId2);
          std::replace(buf.begin(), buf.end(), '/', ' ');
          std::istringstream is(buf);
          is >> messagingId2 >> connId;
        }

        m_iWebsocketService->sendMessage(conMsg.second, connId);
      });

      TRC_DEBUG("Assigned port: " << PAR(m_iWebsocketService->getPort()));
      m_iWebsocketService->registerMessageHandler([&](const std::vector<uint8_t>& msg, const std::string& connId ) -> int {
        return handleMessageFromWebsocket(msg, connId); });

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");

      m_iWebsocketService->unregisterMessageHandler();
      delete m_toMqMessageQueue;

      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "WebsocketMessaging instance deactivate" << std::endl <<
        "******************************"
      );
      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
    }

    void attachInterface(shape::IWebsocketService* iface)
    {
      m_iWebsocketService = iface;
    }

    void detachInterface(shape::IWebsocketService* iface)
    {
      if (m_iWebsocketService == iface) {
        m_iWebsocketService = nullptr;
      }
    }

  };

  ////////////////////////////////////
  // class WebsocketMessaging
  ////////////////////////////////////
  WebsocketMessaging::WebsocketMessaging()
  {
    m_imp = shape_new Imp();
  }

  WebsocketMessaging::~WebsocketMessaging()
  {
    delete m_imp;
  }

  void WebsocketMessaging::registerMessageHandler(MessageHandlerFunc hndl)
  {
    m_imp->registerMessageHandler(hndl);
  }

  void WebsocketMessaging::unregisterMessageHandler()
  {
    m_imp->unregisterMessageHandler();
  }

  void WebsocketMessaging::sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t> & msg)
  {
    m_imp->sendMessage(messaging, msg);
  }

  bool WebsocketMessaging::acceptAsyncMsg() const {
    return m_imp->acceptAsyncMsg();
  }

  const MessagingInstance &WebsocketMessaging::getMessagingInstance() const
  {
    return m_imp->getMessagingInstance();
  }

  //////////////////////////////////////
  void WebsocketMessaging::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void WebsocketMessaging::deactivate()
  {
    m_imp->deactivate();
  }

  void WebsocketMessaging::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void WebsocketMessaging::attachInterface(shape::IWebsocketService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void WebsocketMessaging::detachInterface(shape::IWebsocketService* iface)
  {
    m_imp->detachInterface(iface);
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
