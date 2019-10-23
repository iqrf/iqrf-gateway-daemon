#pragma once

#include "IMessagingService.h"
#include "IMessagingSplitterService.h"
#include "ISchedulerService.h"
#include "TaskQueue.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {
  class SchedulerMessaging : public IMessagingService
  {
  public:
    SchedulerMessaging();
    virtual ~SchedulerMessaging();

    void registerMessageHandler(MessageHandlerFunc hndl) override;
    void unregisterMessageHandler() override;
    void sendMessage(const std::string& messagingId, const std::basic_string<uint8_t> & msg) override;
    void sendMessageExt(const std::string& messagingId, const int nodeAdr, const std::basic_string<uint8_t>& msg) override;
    const std::string & getName() const override;
    bool acceptAsyncMsg() const override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    //void attachInterface(IMessagingSplitterService* iface);
    //void detachInterface(IMessagingSplitterService* iface);

    void attachInterface(ISchedulerService* iface);
    void detachInterface(ISchedulerService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp = nullptr;
  };
}
