#pragma once

#include "IMessagingService.h"
#include "ITestSimulationMessaging.h"
#include "ShapeProperties.h"
#include "ILaunchService.h"
#include "ITraceService.h"

namespace iqrf {

  class IIqrfDpaService;

  class TestSimulationMessaging : public iqrf::IMessagingService, public iqrf::ITestSimulationMessaging
  {
  public:
    TestSimulationMessaging();
    virtual ~TestSimulationMessaging();

    //from iqrf::IMessagingService
    void registerMessageHandler(MessageHandlerFunc hndl) override;
    void unregisterMessageHandler() override;
    void sendMessage(const std::string& messagingId, const std::basic_string<uint8_t> & msg) override;
    void sendMessageExt(const std::string& messagingId, const int nodeAdr, const std::basic_string<uint8_t>& msg) override;
    const std::string & getName() const override;
    bool acceptAsyncMsg() const override;

    //from iqrf::ITestSimulationMessaging
    void pushIncomingMessage(const std::string& msg) override;
    std::string popOutgoingMessage(unsigned millisToWait) override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);
  private:
    class Imp;
    Imp* m_imp = nullptr;
  };

}
