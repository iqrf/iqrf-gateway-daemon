#pragma once

#include "ITestSimulationIqrfChannel.h"
#include "IIqrfChannelService.h"
#include "IJsRenderService.h"
#include "ShapeProperties.h"
#include "ILaunchService.h"
#include "ITraceService.h"

namespace iqrf {

  class TestSimulationIqrfChannel : public IIqrfChannelService, public ITestSimulationIqrfChannel
  {
  public:
    TestSimulationIqrfChannel();
    virtual ~TestSimulationIqrfChannel();

    //iqrf::IIqrfChannelService
    void startListen() override;
    State getState() const override;
    std::unique_ptr<Accessor> getAccess(ReceiveFromFunc receiveFromFunc, AccesType access) override;
    bool hasExclusiveAccess() const override;

    //iqrf::ITestSimulationIqrfChannel
    void pushOutgoingMessage(const std::string& msg, unsigned millisToDelay) override;
    std::string popIncomingMessage(unsigned millisToWait) override;

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
