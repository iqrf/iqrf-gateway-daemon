#define IUdpMessagingService_EXPORTS
#define IMessagingService_EXPORTS

#include "UdpMessaging.h"

#ifdef TRC_CHANNEL
#undefine TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "Trace.h"

#include "iqrf__UdpMessaging.hxx"

TRC_INIT_MODULE(iqrf::UdpMessaging);

const unsigned IQRF_MQ_BUFFER_SIZE = 64 * 1024;

namespace iqrf {
  UdpMessaging::UdpMessaging()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  UdpMessaging::~UdpMessaging()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  void UdpMessaging::registerMessageHandler(MessageHandlerFunc hndl)
  {
    TRC_FUNCTION_ENTER("");
    m_messageHandlerFunc = hndl;
    TRC_FUNCTION_LEAVE("")
  }

  void UdpMessaging::unregisterMessageHandler()
  {
    TRC_FUNCTION_ENTER("");
    m_messageHandlerFunc = IMessagingService::MessageHandlerFunc();
    TRC_FUNCTION_LEAVE("")
  }

  void UdpMessaging::sendMessage(const std::basic_string<uint8_t> & msg)
  {
    TRC_FUNCTION_ENTER("");
    TRC_DEBUG(MEM_HEX_CHAR(msg.data(), msg.size()));
    m_toUdpMessageQueue->pushToQueue(msg);
    TRC_FUNCTION_LEAVE("")
  }

  int UdpMessaging::handleMessageFromUdp(const std::basic_string<uint8_t> & message)
  {
    TRC_DEBUG("==================================" << std::endl <<
      "Received from UDP: " << std::endl << MEM_HEX_CHAR(message.data(), message.size()));

    if (m_messageHandlerFunc)
      m_messageHandlerFunc(m_name, std::vector<uint8_t>(message.data(), message.data() + message.size()));

    return 0;
  }

  //////////////////////////////////////
  void UdpMessaging::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "UdpMessaging instance activate" << std::endl <<
      "******************************"
    );

    props->getMemberAsString("instance", m_name);
    props->getMemberAsInt("RemotePort", m_remotePort);
    props->getMemberAsInt("LocalPort", m_localPort);

    m_udpChannel = shape_new UdpChannel(m_remotePort, m_localPort, IQRF_MQ_BUFFER_SIZE);

    m_toUdpMessageQueue = shape_new TaskQueue<std::basic_string<uint8_t>>([&](const std::basic_string<uint8_t>& msg) {
      m_udpChannel->sendTo(msg);
    });

    m_udpChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
      return handleMessageFromUdp(msg); });

    TRC_FUNCTION_LEAVE("")
  }

  void UdpMessaging::deactivate()
  {
    TRC_FUNCTION_ENTER("");
    
    m_udpChannel->unregisterReceiveFromHandler();
    
    delete m_udpChannel;
    delete m_toUdpMessageQueue;

    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "UdpMessaging instance deactivate" << std::endl <<
      "******************************"
    );
    TRC_FUNCTION_LEAVE("")
  }

  void UdpMessaging::modify(const shape::Properties *props)
  {
  }

  void UdpMessaging::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void UdpMessaging::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
