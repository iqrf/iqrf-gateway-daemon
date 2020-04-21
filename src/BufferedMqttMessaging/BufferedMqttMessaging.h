#pragma once

#include "IMessagingService.h"
#include "IMqttService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {
  class BufferedMqttMessagingImpl;

  class BufferedMqttMessaging : public IMessagingService
  {
  public:
    BufferedMqttMessaging();
    virtual ~BufferedMqttMessaging();
    
    void registerMessageHandler(MessageHandlerFunc hndl) override;
    void unregisterMessageHandler() override;
    void sendMessage(const std::string& messagingId, const std::basic_string<uint8_t> & msg) override;
    const std::string & getName() const override;
    bool acceptAsyncMsg() const override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::IMqttService* iface);
    void detachInterface(shape::IMqttService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    BufferedMqttMessagingImpl* m_impl;
  };
}
