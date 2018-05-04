#pragma once

#include "IMessagingService.h"
#include "TaskQueue.h"
#include "ShapeProperties.h"
#include "IWebsocketService.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {
  class WebsocketMessaging : public IMessagingService
  {
  public:
    WebsocketMessaging();
    virtual ~WebsocketMessaging();

    void registerMessageHandler(MessageHandlerFunc hndl) override;
    void unregisterMessageHandler() override;
    void sendMessage(const std::basic_string<uint8_t> & msg) override;
    const std::string & getName() const override;
    bool acceptAsyncMsg() const override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::IWebsocketService* iface);
    void detachInterface(shape::IWebsocketService* iface);
    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp = nullptr;
  };
}
