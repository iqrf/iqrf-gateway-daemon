#define IMessagingService_EXPORTS

#include "MqMessaging.h"

#ifdef TRC_CHANNEL
#undefine TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "Trace.h"

#include "iqrf__MqMessaging.hxx"

TRC_INIT_MODULE(iqrf::MqMessaging);

//TODO workaround old tracing 
#include "IqrfLogging.h"
TRC_INIT();

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

  void MqMessaging::sendMessage(const std::basic_string<uint8_t> & msg)
  {
    TRC_FUNCTION_ENTER("");
    TRC_DEBUG(MEM_HEX_CHAR(msg.data(), msg.size()));
    m_toMqMessageQueue->pushToQueue(msg);
    TRC_FUNCTION_LEAVE("")
  }

  int MqMessaging::handleMessageFromMq(const ustring& message)
  {
    TRC_DEBUG("==================================" << std::endl <<
      "Received from MQ: " << std::endl << MEM_HEX_CHAR(message.data(), message.size()));

    if (m_messageHandlerFunc)
      m_messageHandlerFunc(m_name, std::vector<uint8_t>(message.data(), message.data() + message.size()));

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

    //TODO workaround old tracing 
    TRC_START("TraceOldMqChannel.txt", iqrf::Level::dbg, TRC_DEFAULT_FILE_MAXSIZE);

    props->getMemberAsString("instance", m_name);
    props->getMemberAsString("LocalMqName", m_localMqName);
    props->getMemberAsString("RemoteMqName", m_remoteMqName);

    m_mqChannel = ant_new MqChannel(m_remoteMqName, m_localMqName, IQRF_MQ_BUFFER_SIZE, true);

    m_toMqMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
      m_mqChannel->sendTo(msg);
    });

    m_mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
      return handleMessageFromMq(msg); });

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
