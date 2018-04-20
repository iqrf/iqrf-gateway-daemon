#pragma once

#include "IUdpMessagingService.h"
#include "TaskQueue.h"
#include "UdpChannel.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {
  class UdpMessaging : public IUdpMessagingService
  {
  public:
    UdpMessaging();
    virtual ~UdpMessaging();

    void registerMessageHandler(MessageHandlerFunc hndl) override;
    void unregisterMessageHandler() override;
    void sendMessage(const std::basic_string<uint8_t> & msg) override;
    const std::string & getName() const override { return m_name; }

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    int handleMessageFromUdp(const std::basic_string<uint8_t> & message);

    std::string m_name;
    UdpChannel* m_udpChannel = nullptr;
    TaskQueue<std::basic_string<uint8_t>>* m_toUdpMessageQueue = nullptr;

    int m_remotePort = 55000;
    int m_localPort = 55300;

    IMessagingService::MessageHandlerFunc m_messageHandlerFunc;
  };
}
