#pragma once

#include "IMessagingService.h"
#include "TaskQueue.h"
#include "MqChannel.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {
  class MqMessaging : public IMessagingService
  {
  public:
    MqMessaging();
    virtual ~MqMessaging();

    void registerMessageHandler(MessageHandlerFunc hndl) override;
    void unregisterMessageHandler() override;
    void sendMessage(const std::string& messagingId, const std::basic_string<uint8_t> & msg) override;
    void sendMessageExt(const std::string& messagingId, const int nodeAdr, const std::basic_string<uint8_t>& msg) override;
    const std::string & getName() const override { return m_name; }
    bool acceptAsyncMsg() const override { return m_acceptAsyncMsg; }

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    int handleMessageFromMq(const std::basic_string<uint8_t> & mqMessage);

    std::string m_name;
    bool m_acceptAsyncMsg = false;

    MqChannel* m_mqChannel = nullptr;
    TaskQueue<std::basic_string<uint8_t>>* m_toMqMessageQueue = nullptr;

    std::string m_localMqName = "iqrf-daemon-110";
    std::string m_remoteMqName = "iqrf-daemon-100";

    IMessagingService::MessageHandlerFunc m_messageHandlerFunc;
  };
}
