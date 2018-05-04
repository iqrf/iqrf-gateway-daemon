#pragma once

#include "IMessagingService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {
  class MqttMessagingImpl;

  class MqttMessaging : public IMessagingService
  {
  public:
    MqttMessaging();
    virtual ~MqttMessaging();
    
    void registerMessageHandler(MessageHandlerFunc hndl) override;
    void unregisterMessageHandler() override;
    void sendMessage(const std::basic_string<uint8_t> & msg) override;
    const std::string & getName() const override;
    bool acceptAsyncMsg() const override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    MqttMessagingImpl* m_impl;
  };
}
