/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define IMessagingService_EXPORTS

#include "BufferedMqttMessaging.h"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "Trace.h"

#include "iqrf__BufferedMqttMessaging.hxx"

TRC_INIT_MODULE(iqrf::BufferedMqttMessaging)

namespace iqrf {

  typedef std::basic_string<uint8_t> ustring;

  class BufferedMqttMessagingImpl {

  private:
    shape::IMqttService* m_iMqttService = nullptr;

    std::string m_mqttClientId;
    std::string m_mqttTopicRequest;
    std::string m_mqttTopicResponse;
    bool m_acceptAsyncMsg = false;
    MessagingInstance m_messagingInstance = MessagingInstance(MessagingType::BUFFERED_MQTT);
    IMessagingService::MessageHandlerFunc m_messageHandlerFunc;

  public:
    BufferedMqttMessagingImpl()
    {}

    //------------------------
    ~BufferedMqttMessagingImpl()
    {}

    //------------------------
    void start()
    {
      TRC_FUNCTION_ENTER("");

      m_iMqttService->create(m_mqttClientId);
      m_iMqttService->registerMessageStrHandler([&](const std::string& topic, const std::string & msg)
      {
        (void)topic;
        if (m_messageHandlerFunc) {
          m_messageHandlerFunc(m_messagingInstance, std::vector<uint8_t>((uint8_t*)msg.data(), (uint8_t*)(msg.data() + msg.size())));
        }
      });

      m_iMqttService->registerOnConnectHandler([&]()
      {
        m_iMqttService->subscribe(m_mqttTopicRequest);
      });

      m_iMqttService->connect();

      TRC_FUNCTION_LEAVE("");
    }

    //------------------------
    void stop()
    {
      TRC_FUNCTION_ENTER("");

      m_iMqttService->unregisterMessageStrHandler();
      m_iMqttService->unregisterOnConnectHandler();
      m_iMqttService->disconnect();

      TRC_FUNCTION_LEAVE("");
    }

    //------------------------
    bool acceptAsyncMsg() const {
      return m_acceptAsyncMsg;
    }

    //------------------------
    const MessagingInstance &getMessagingInstance() const
    {
      return m_messagingInstance;
    }

    //------------------------
    void registerMessageHandler(IMessagingService::MessageHandlerFunc hndl) {
      TRC_FUNCTION_ENTER("");
      m_messageHandlerFunc = hndl;
      TRC_FUNCTION_LEAVE("");
    }

    //------------------------
    void unregisterMessageHandler() {
      TRC_FUNCTION_ENTER("");
      m_messageHandlerFunc = IMessagingService::MessageHandlerFunc();
      TRC_FUNCTION_LEAVE("");
    }

    //------------------------
    void sendMessage(const ustring& msg) {
      TRC_FUNCTION_ENTER("");
      m_iMqttService->publish(m_mqttTopicResponse, std::vector<uint8_t>(msg.data(), msg.data() + msg.size()));
      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "***************************************" << std::endl <<
        "BufferedMqttMessaging instance activate" << std::endl <<
        "***************************************"
      );

      modify(props);

      start();

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "*****************************************" << std::endl <<
        "BufferedMqttMessaging instance deactivate" << std::endl <<
        "*****************************************"
      );

      stop();

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      std::string instanceName;

      props->getMemberAsString("instance", instanceName);
      props->getMemberAsString("ClientId", m_mqttClientId);
      props->getMemberAsString("TopicRequest", m_mqttTopicRequest);
      props->getMemberAsString("TopicResponse", m_mqttTopicResponse);
      props->getMemberAsBool("acceptAsyncMsg", m_acceptAsyncMsg);

      m_messagingInstance.instance = instanceName;
      TRC_FUNCTION_LEAVE("");
    }

    void attachInterface(shape::IMqttService* iface)
    {
      TRC_FUNCTION_ENTER("");
      m_iMqttService = iface;
      TRC_FUNCTION_LEAVE("")
    }

    void detachInterface(shape::IMqttService* iface)
    {
      TRC_FUNCTION_ENTER("");
      if (m_iMqttService == iface) {
        m_iMqttService = nullptr;
      }
      TRC_FUNCTION_LEAVE("")
    }
  };

  //////////////////
  // MqttMessaging implementation
  //////////////////

  BufferedMqttMessaging::BufferedMqttMessaging()
  {
    TRC_FUNCTION_ENTER("");
    m_impl = shape_new BufferedMqttMessagingImpl();
    TRC_FUNCTION_LEAVE("")
  }

  BufferedMqttMessaging::~BufferedMqttMessaging()
  {
    TRC_FUNCTION_ENTER("");
    delete m_impl;
    TRC_FUNCTION_LEAVE("")
  }

  void BufferedMqttMessaging::registerMessageHandler(MessageHandlerFunc hndl)
  {
    m_impl->registerMessageHandler(hndl);
  }

  void BufferedMqttMessaging::unregisterMessageHandler()
  {
    m_impl->unregisterMessageHandler();
  }

  void BufferedMqttMessaging::sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t> & msg)
  {
    TRC_FUNCTION_LEAVE(PAR(messaging.instance));
    m_impl->sendMessage(msg);
    TRC_FUNCTION_LEAVE("");
  }

  bool BufferedMqttMessaging::acceptAsyncMsg() const {
    return m_impl->acceptAsyncMsg();
  }

  const MessagingInstance &BufferedMqttMessaging::getMessagingInstance() const
  {
    return m_impl->getMessagingInstance();
  }

  void BufferedMqttMessaging::activate(const shape::Properties *props)
  {
    return m_impl->activate(props);
  }

  void BufferedMqttMessaging::deactivate()
  {
    return m_impl->deactivate();
  }

  void BufferedMqttMessaging::modify(const shape::Properties *props)
  {
    m_impl->modify(props);
  }

  void BufferedMqttMessaging::attachInterface(shape::IMqttService* iface)
  {
    m_impl->attachInterface(iface);
  }

  void BufferedMqttMessaging::detachInterface(shape::IMqttService* iface)
  {
    m_impl->detachInterface(iface);
  }

  void BufferedMqttMessaging::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void BufferedMqttMessaging::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }
}
