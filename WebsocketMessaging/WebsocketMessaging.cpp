#define IMessagingService_EXPORTS

#include "WebsocketMessaging.h"
#include "Trace.h"
#include <vector>

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "iqrf__WebsocketMessaging.hxx"

TRC_INIT_MODULE(iqrf::WebsocketMessaging);

const unsigned IQRF_MQ_BUFFER_SIZE = 64 * 1024;

namespace iqrf {

  ////////////////////////////////////
  // class WebsocketMessaging::Imp
  ////////////////////////////////////
  class WebsocketMessaging::Imp
  {
  private:
    std::string m_name;
    bool m_acceptAsyncMsg = false;

    shape::IWebsocketService* m_iWebsocketService = nullptr;

    TaskQueue<std::vector<uint8_t>>* m_toMqMessageQueue = nullptr;

    IMessagingService::MessageHandlerFunc m_messageHandlerFunc;

    int handleMessageFromWebsocket(const std::vector<uint8_t>& message)
    {
      TRC_DEBUG("==================================" << std::endl <<
        "Received from Websocket: " << std::endl << MEM_HEX_CHAR(message.data(), message.size()));

      if (m_messageHandlerFunc)
        m_messageHandlerFunc(m_name, message);

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

    void sendMessage(const std::basic_string<uint8_t> & msg)
    {
      TRC_FUNCTION_ENTER("");
      TRC_DEBUG(MEM_HEX_CHAR(msg.data(), msg.size()));
      m_toMqMessageQueue->pushToQueue(std::vector<uint8_t>(msg.data(), msg.data() + msg.size()));
      TRC_FUNCTION_LEAVE("")
    }

    const std::string& getName() const
    {
      return m_name;
    }

    bool acceptAsyncMsg() const
    {
      return m_acceptAsyncMsg;
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "WebsocketMessaging instance activate" << std::endl <<
        "******************************"
      );

      props->getMemberAsString("instance", m_name);
      props->getMemberAsBool("acceptAsyncMsg", m_acceptAsyncMsg);

      m_toMqMessageQueue = shape_new TaskQueue<std::vector<uint8_t>>([&](const std::vector<uint8_t>& msg) {
        m_iWebsocketService->sendMessage(msg);
      });

      TRC_DEBUG("Assigned port: " << PAR(m_iWebsocketService->getPort()));
      m_iWebsocketService->registerMessageHandler([&](const std::vector<uint8_t>& msg) -> int {
        return handleMessageFromWebsocket(msg); });

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

  void WebsocketMessaging::sendMessage(const std::basic_string<uint8_t> & msg)
  {
    m_imp->sendMessage(msg);
  }

  const std::string& WebsocketMessaging::getName() const
  {
    return m_imp->getName();
  }

  bool WebsocketMessaging::acceptAsyncMsg() const
  {
    return m_imp->acceptAsyncMsg();
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
