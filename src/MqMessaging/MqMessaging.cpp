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

#include "MqMessaging.h"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "Trace.h"

#include "iqrf__MqMessaging.hxx"

TRC_INIT_MODULE(iqrf::MqMessaging)

const unsigned IQRF_MQ_BUFFER_SIZE = 64 * 1024;

namespace iqrf {
  MqMessaging::MqMessaging()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  MqMessaging::~MqMessaging()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  void MqMessaging::registerMessageHandler(MessageHandlerFunc hndl)
  {
    TRC_FUNCTION_ENTER("");
    m_messageHandlerFunc = hndl;
    TRC_FUNCTION_LEAVE("")
  }

  void MqMessaging::unregisterMessageHandler()
  {
    TRC_FUNCTION_ENTER("");
    m_messageHandlerFunc = IMessagingService::MessageHandlerFunc();
    TRC_FUNCTION_LEAVE("")
  }

  void MqMessaging::sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t> & msg)
  {
    TRC_FUNCTION_ENTER(PAR(messaging.instance));
    TRC_DEBUG(MEM_HEX_CHAR(msg.data(), msg.size()));
    m_toMqMessageQueue->pushToQueue(msg);
    TRC_FUNCTION_LEAVE("")
  }

  bool MqMessaging::acceptAsyncMsg() const {
    return m_acceptAsyncMsg;
  }

  const MessagingInstance &MqMessaging::getMessagingInstance() const
  {
    return m_messagingInstance;
  }

  int MqMessaging::handleMessageFromMq(const ustring& message)
  {
    TRC_DEBUG("==================================" << std::endl <<
      "Received from MQ: " << std::endl << MEM_HEX_CHAR(message.data(), message.size()));

    if (m_messageHandlerFunc) {
      m_messageHandlerFunc(m_messagingInstance, std::vector<uint8_t>(message.data(), message.data() + message.size()));
    }

    return 0;
  }

  //////////////////////////////////////
  void MqMessaging::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "MqMessaging instance activate" << std::endl <<
      "******************************"
    );

    std::string instanceName;

    const rapidjson::Document &doc = props->getAsJson();
    props->getMemberAsString("instance", instanceName);
    props->getMemberAsString("LocalMqName", m_localMqName);
    props->getMemberAsString("RemoteMqName", m_remoteMqName);
    props->getMemberAsBool("acceptAsyncMsg", m_acceptAsyncMsg);
    m_timeout = (uint8_t)rapidjson::Pointer("/timeout").Get(doc)->GetUint();

    m_mqChannel = shape_new MqChannel(m_remoteMqName, m_localMqName, m_timeout, IQRF_MQ_BUFFER_SIZE, true);

    m_messagingInstance.instance = instanceName;

    m_toMqMessageQueue = shape_new TaskQueue<ustring>([&](const ustring& msg) {
      m_mqChannel->sendTo(msg);
    });

    m_mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
      return handleMessageFromMq(msg);
    });

    TRC_FUNCTION_LEAVE("")
  }

  void MqMessaging::deactivate()
  {
    TRC_FUNCTION_ENTER("");

    m_mqChannel->unregisterReceiveFromHandler();
    delete m_mqChannel;
    delete m_toMqMessageQueue;

    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "MqMessaging instance deactivate" << std::endl <<
      "******************************"
    );
    TRC_FUNCTION_LEAVE("")
  }

  void MqMessaging::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void MqMessaging::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void MqMessaging::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
